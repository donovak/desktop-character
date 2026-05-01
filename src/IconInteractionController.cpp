#include "IconInteractionController.h"

#include "DebugLog.h"

#include <limits>
#include <shellapi.h>
#include <string>

namespace {
constexpr int INTERACTION_PADDING = 14;
constexpr int FAST_ROLL_BUMP_PADDING = 3;
constexpr auto INTERACTION_COOLDOWN = std::chrono::milliseconds(750);
constexpr auto FAST_ROLL_BUMP_HIGHLIGHT_DURATION = std::chrono::milliseconds(260);

std::wstring hresultToHex(HRESULT result)
{
    wchar_t buffer[16] {};
    swprintf_s(buffer, L"0x%08X", static_cast<unsigned int>(result));
    return buffer;
}
}

IconInteractionController::IconInteractionController(bool dryRunInteractions)
    : m_dryRunInteractions(dryRunInteractions)
{
    if (m_dryRunInteractions) {
        debugLog(L"Icon interactions are running in dry-run mode.");
    }
}

void IconInteractionController::updateInteractableIcon(const std::vector<DesktopIcon>& icons, const RECT& characterScreenBounds)
{
    int bestIndex = -1;
    int bestDistanceSquared = std::numeric_limits<int>::max();

    for (std::size_t index = 0; index < icons.size(); ++index) {
        const RECT interactionBounds = inflatedRect(icons[index].screenBounds, INTERACTION_PADDING);
        if (!rectsIntersect(characterScreenBounds, interactionBounds)) {
            continue;
        }

        const int distanceSquared = centerDistanceSquared(characterScreenBounds, icons[index].screenBounds);
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestIndex = static_cast<int>(index);
        }
    }

    m_interactableIconIndex = bestIndex;

    const std::wstring currentId =
        m_interactableIconIndex >= 0 ? icons[static_cast<std::size_t>(m_interactableIconIndex)].debugIdentifier : std::wstring {};

    if (currentId != m_lastInteractableId) {
        m_lastInteractableId = currentId;

        if (m_interactableIconIndex >= 0) {
            debugLog(std::wstring(L"Interactable icon: ")
                + iconLabel(icons[static_cast<std::size_t>(m_interactableIconIndex)]));
        } else {
            debugLog(L"No icon currently interactable.");
        }
    }
}

void IconInteractionController::updateFastRollCollision(const std::vector<DesktopIcon>& icons, const RECT& characterScreenBounds, bool isFastRolling)
{
    const auto now = std::chrono::steady_clock::now();

    if (!isFastRolling) {
        if (now >= m_bumpHighlightUntil) {
            m_bumpedIconIndex = -1;
            m_lastBumpedIconId.clear();
        }
        return;
    }

    int bestIndex = -1;
    int bestDistanceSquared = std::numeric_limits<int>::max();

    for (std::size_t index = 0; index < icons.size(); ++index) {
        const RECT bumpBounds = inflatedRect(icons[index].screenBounds, FAST_ROLL_BUMP_PADDING);
        if (!rectsIntersect(characterScreenBounds, bumpBounds)) {
            continue;
        }

        const int distanceSquared = centerDistanceSquared(characterScreenBounds, icons[index].screenBounds);
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestIndex = static_cast<int>(index);
        }
    }

    if (bestIndex < 0) {
        if (now >= m_bumpHighlightUntil) {
            m_bumpedIconIndex = -1;
            m_lastBumpedIconId.clear();
        }
        return;
    }

    m_bumpedIconIndex = bestIndex;
    m_bumpHighlightUntil = now + FAST_ROLL_BUMP_HIGHLIGHT_DURATION;

    const DesktopIcon& icon = icons[static_cast<std::size_t>(bestIndex)];
    if (icon.debugIdentifier != m_lastBumpedIconId) {
        m_lastBumpedIconId = icon.debugIdentifier;
        debugLog(std::wstring(L"Fast-roll visual collision with icon: ") + iconLabel(icon));
    }
}

void IconInteractionController::noteIconCollision(const std::vector<DesktopIcon>& icons, int iconIndex, bool strongCollision)
{
    if (iconIndex < 0 || static_cast<std::size_t>(iconIndex) >= icons.size()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    m_bumpedIconIndex = iconIndex;
    m_bumpHighlightUntil = now + FAST_ROLL_BUMP_HIGHLIGHT_DURATION;

    const DesktopIcon& icon = icons[static_cast<std::size_t>(iconIndex)];
    const std::wstring collisionId = icon.debugIdentifier + (strongCollision ? L":fast" : L":regular");
    if (collisionId == m_lastBumpedIconId) {
        return;
    }

    m_lastBumpedIconId = collisionId;
    debugLog(std::wstring(strongCollision ? L"Fast-roll bump collision with icon: " : L"Icon collision with icon: ")
        + iconLabel(icon));
}

bool IconInteractionController::tryInteract(const std::vector<DesktopIcon>& icons, std::chrono::milliseconds launchDelay)
{
    if (m_interactableIconIndex < 0 || static_cast<std::size_t>(m_interactableIconIndex) >= icons.size()) {
        debugLog(L"Icon interaction attempted, but no icon is interactable.");
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastInteractionTime < INTERACTION_COOLDOWN) {
        debugLog(L"Icon interaction skipped due to cooldown.");
        return false;
    }

    m_lastInteractionTime = now;

    const DesktopIcon& icon = icons[static_cast<std::size_t>(m_interactableIconIndex)];
    debugLog(std::wstring(L"Icon interaction attempted: ") + iconLabel(icon));

    if (icon.filesystemPath.empty()) {
        debugLog(std::wstring(L"Icon interaction skipped; no safe filesystem path for ") + iconLabel(icon));
        return true;
    }

    m_hasPendingLaunch = true;
    m_pendingLaunchPath = icon.filesystemPath;
    m_pendingLaunchLabel = iconLabel(icon);
    m_pendingLaunchTime = now + launchDelay;
    debugLog(std::wstring(L"Icon launch scheduled after animation lead-in: ") + m_pendingLaunchLabel);
    return true;
}

void IconInteractionController::updatePendingLaunch()
{
    if (!m_hasPendingLaunch || std::chrono::steady_clock::now() < m_pendingLaunchTime) {
        return;
    }

    m_hasPendingLaunch = false;

    if (m_dryRunInteractions) {
        debugLog(std::wstring(L"Dry-run interaction: would open ") + m_pendingLaunchPath);
        return;
    }

    DesktopIcon icon {};
    icon.displayName = m_pendingLaunchLabel;
    icon.filesystemPath = m_pendingLaunchPath;
    if (launchIcon(icon)) {
        debugLog(std::wstring(L"Icon launch succeeded: ") + m_pendingLaunchPath);
    }
}

int IconInteractionController::interactableIconIndex() const
{
    return m_interactableIconIndex;
}

int IconInteractionController::bumpedIconIndex() const
{
    return m_bumpedIconIndex;
}

bool IconInteractionController::rectsIntersect(const RECT& a, const RECT& b)
{
    return a.left < b.right
        && a.right > b.left
        && a.top < b.bottom
        && a.bottom > b.top;
}

RECT IconInteractionController::inflatedRect(const RECT& rect, int padding)
{
    return {
        rect.left - padding,
        rect.top - padding,
        rect.right + padding,
        rect.bottom + padding
    };
}

int IconInteractionController::centerDistanceSquared(const RECT& a, const RECT& b)
{
    const int centerAX = a.left + ((a.right - a.left) / 2);
    const int centerAY = a.top + ((a.bottom - a.top) / 2);
    const int centerBX = b.left + ((b.right - b.left) / 2);
    const int centerBY = b.top + ((b.bottom - b.top) / 2);
    const int deltaX = centerAX - centerBX;
    const int deltaY = centerAY - centerBY;
    return (deltaX * deltaX) + (deltaY * deltaY);
}

std::wstring IconInteractionController::iconLabel(const DesktopIcon& icon)
{
    if (!icon.displayName.empty()) {
        return icon.displayName;
    }

    if (!icon.debugIdentifier.empty()) {
        return icon.debugIdentifier;
    }

    return L"(unknown desktop icon)";
}

bool IconInteractionController::launchIcon(const DesktopIcon& icon) const
{
    SHELLEXECUTEINFOW executeInfo {};
    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    executeInfo.lpVerb = L"open";
    executeInfo.lpFile = icon.filesystemPath.c_str();
    executeInfo.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&executeInfo)) {
        const HRESULT result = HRESULT_FROM_WIN32(GetLastError());
        debugLog(std::wstring(L"Icon launch failed for ")
            + icon.filesystemPath
            + L", hr="
            + hresultToHex(result));
        return false;
    }

    if (executeInfo.hProcess != nullptr) {
        CloseHandle(executeInfo.hProcess);
    }

    return true;
}
