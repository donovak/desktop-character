#include "Input.h"

#include "DebugLog.h"

#include <atomic>
#include <array>
#include <string>
#include <windows.h>

namespace {
constexpr float DOUBLE_TAP_WINDOW_SECONDS = 0.28f;

std::atomic_bool g_controlModeEnabled = false;
HHOOK g_keyboardHook = nullptr;
std::array<std::atomic_bool, 256> g_hookKeyDown {};

bool isKeyDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

bool isHookKeyDown(int virtualKey)
{
    if (virtualKey < 0 || virtualKey >= static_cast<int>(g_hookKeyDown.size())) {
        return isKeyDown(virtualKey);
    }

    return g_hookKeyDown[static_cast<std::size_t>(virtualKey)].load(std::memory_order_relaxed);
}

bool shouldSuppressKey(DWORD virtualKey)
{
    if (virtualKey >= VK_F2 && virtualKey <= VK_F7) {
        return true;
    }

    switch (virtualKey) {
    case 'W':
    case 'A':
    case 'S':
    case 'D':
    case 'E':
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_SPACE:
        return true;
    default:
        return false;
    }
}

LRESULT CALLBACK keyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION) {
        const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        const bool isKeyMessage = wParam == WM_KEYDOWN
            || wParam == WM_KEYUP
            || wParam == WM_SYSKEYDOWN
            || wParam == WM_SYSKEYUP;

        if (isKeyMessage && event != nullptr && event->vkCode < g_hookKeyDown.size()) {
            const bool isDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
            g_hookKeyDown[static_cast<std::size_t>(event->vkCode)].store(isDown, std::memory_order_relaxed);
        }

        if (isKeyMessage
            && event != nullptr
            && g_controlModeEnabled.load(std::memory_order_relaxed)
            && shouldSuppressKey(event->vkCode)) {
            return 1;
        }
    }

    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}
}

Input::Input()
{
    installKeyboardHook();
    setControlModeEnabled(true);
}

Input::~Input()
{
    uninstallKeyboardHook();
}

void Input::update()
{
    m_movementDirection = {};
    const bool useCapturedKeys = m_controlModeEnabled && g_keyboardHook != nullptr;
    const auto keyDown = [useCapturedKeys](int virtualKey) {
        return useCapturedKeys ? isHookKeyDown(virtualKey) : isKeyDown(virtualKey);
    };

    m_shouldExit = keyDown(VK_ESCAPE);
    m_shouldToggleIconDebugOverlay = false;
    m_shouldToggleIconHoverBounds = false;
    m_shouldToggleIconImageBounds = false;
    m_shouldToggleIconAnchors = false;
    m_shouldToggleIconLabels = false;
    m_shouldRefreshDesktopIcons = false;
    m_shouldInteract = false;
    m_shouldJump = false;
    m_pendingFastRollDashDirection = 0;

    const bool isF2Down = keyDown(VK_F2);
    const bool isF3Down = keyDown(VK_F3);
    const bool isF4Down = keyDown(VK_F4);
    const bool isF5Down = keyDown(VK_F5);
    const bool isF6Down = keyDown(VK_F6);
    const bool isF7Down = keyDown(VK_F7);
    const bool isF8Down = keyDown(VK_F8);
    const bool isSpaceDown = keyDown(VK_SPACE);
    const bool isEDown = keyDown('E');

    if (isF8Down && !m_wasF8Down) {
        setControlModeEnabled(!m_controlModeEnabled);
    }

    const bool isLeftDashDown = keyDown('A') || keyDown(VK_LEFT);
    const bool isRightDashDown = keyDown('D') || keyDown(VK_RIGHT);

    if (m_controlModeEnabled) {
        m_shouldToggleIconDebugOverlay = isF2Down && !m_wasF2Down;
        m_shouldToggleIconHoverBounds = isF3Down && !m_wasF3Down;
        m_shouldToggleIconImageBounds = isF4Down && !m_wasF4Down;
        m_shouldRefreshDesktopIcons = isF5Down && !m_wasF5Down;
        m_shouldToggleIconAnchors = isF6Down && !m_wasF6Down;
        m_shouldToggleIconLabels = isF7Down && !m_wasF7Down;
        m_shouldJump = isSpaceDown && !m_wasSpaceDown;
        m_shouldInteract = isEDown && !m_wasEDown;
        updateTapDash(isLeftDashDown, isRightDashDown);
    }

    m_wasF2Down = isF2Down;
    m_wasF3Down = isF3Down;
    m_wasF4Down = isF4Down;
    m_wasF5Down = isF5Down;
    m_wasF6Down = isF6Down;
    m_wasF7Down = isF7Down;
    m_wasF8Down = isF8Down;
    m_wasSpaceDown = isSpaceDown;
    m_wasEDown = isEDown;
    m_wasLeftDashDown = isLeftDashDown;
    m_wasRightDashDown = isRightDashDown;

    if (!m_controlModeEnabled) {
        return;
    }

    if (keyDown('A') || keyDown(VK_LEFT)) {
        m_movementDirection.x -= 1.0f;
    }

    if (keyDown('D') || keyDown(VK_RIGHT)) {
        m_movementDirection.x += 1.0f;
    }

    if (keyDown('W') || keyDown(VK_UP)) {
        m_movementDirection.y -= 1.0f;
    }

    if (keyDown('S') || keyDown(VK_DOWN)) {
        m_movementDirection.y += 1.0f;
    }
}

Vector2 Input::movementDirection() const
{
    return m_movementDirection;
}

bool Input::shouldExit() const
{
    return m_shouldExit;
}

bool Input::isControlModeEnabled() const
{
    return m_controlModeEnabled;
}

bool Input::shouldToggleIconDebugOverlay() const
{
    return m_shouldToggleIconDebugOverlay;
}

bool Input::shouldToggleIconHoverBounds() const
{
    return m_shouldToggleIconHoverBounds;
}

bool Input::shouldToggleIconImageBounds() const
{
    return m_shouldToggleIconImageBounds;
}

bool Input::shouldToggleIconAnchors() const
{
    return m_shouldToggleIconAnchors;
}

bool Input::shouldToggleIconLabels() const
{
    return m_shouldToggleIconLabels;
}

bool Input::shouldRefreshDesktopIcons() const
{
    return m_shouldRefreshDesktopIcons;
}

bool Input::shouldInteract() const
{
    return m_shouldInteract;
}

bool Input::shouldJump() const
{
    return m_shouldJump;
}

int Input::consumeFastRollDashDirection()
{
    const int direction = m_pendingFastRollDashDirection;
    m_pendingFastRollDashDirection = 0;
    return direction;
}

void Input::installKeyboardHook()
{
    if (g_keyboardHook != nullptr) {
        return;
    }

    g_keyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        keyboardHookProc,
        GetModuleHandleW(nullptr),
        0);

    if (g_keyboardHook == nullptr) {
        debugLog(std::wstring(L"Failed to install low-level keyboard hook; control mode will not suppress keys. error=")
            + std::to_wstring(GetLastError()));
    }
}

void Input::uninstallKeyboardHook()
{
    g_controlModeEnabled.store(false, std::memory_order_relaxed);

    if (g_keyboardHook == nullptr) {
        return;
    }

    UnhookWindowsHookEx(g_keyboardHook);
    g_keyboardHook = nullptr;
}

void Input::updateTapDash(bool isLeftDown, bool isRightDown)
{
    const auto now = std::chrono::steady_clock::now();
    const auto doubleTapWindow = std::chrono::duration<float>(DOUBLE_TAP_WINDOW_SECONDS);

    if (isLeftDown && !m_wasLeftDashDown) {
        if (m_lastLeftTapTime.time_since_epoch().count() != 0
            && now - m_lastLeftTapTime <= doubleTapWindow) {
            m_pendingFastRollDashDirection = -1;
        }

        m_lastLeftTapTime = now;
    }

    if (isRightDown && !m_wasRightDashDown) {
        if (m_lastRightTapTime.time_since_epoch().count() != 0
            && now - m_lastRightTapTime <= doubleTapWindow) {
            m_pendingFastRollDashDirection = 1;
        }

        m_lastRightTapTime = now;
    }
}

void Input::setControlModeEnabled(bool enabled)
{
    if (m_controlModeEnabled == enabled) {
        return;
    }

    m_controlModeEnabled = enabled;
    g_controlModeEnabled.store(enabled, std::memory_order_relaxed);
    debugLog(enabled
        ? L"Control mode enabled; gameplay keys are suppressed from background apps."
        : L"Control mode disabled; gameplay input is paused and keys are no longer suppressed.");
}
