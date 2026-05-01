#pragma once

#include "DesktopIconService.h"

#include <chrono>
#include <string>
#include <vector>
#include <windows.h>

class IconInteractionController {
public:
    explicit IconInteractionController(bool dryRunInteractions);

    void updateInteractableIcon(const std::vector<DesktopIcon>& icons, const RECT& characterScreenBounds);
    void updateFastRollCollision(const std::vector<DesktopIcon>& icons, const RECT& characterScreenBounds, bool isFastRolling);
    void noteIconCollision(const std::vector<DesktopIcon>& icons, int iconIndex, bool strongCollision);
    bool tryInteract(const std::vector<DesktopIcon>& icons, std::chrono::milliseconds launchDelay);
    void updatePendingLaunch();
    int interactableIconIndex() const;
    int bumpedIconIndex() const;

private:
    static bool rectsIntersect(const RECT& a, const RECT& b);
    static RECT inflatedRect(const RECT& rect, int padding);
    static int centerDistanceSquared(const RECT& a, const RECT& b);
    static std::wstring iconLabel(const DesktopIcon& icon);

    bool launchIcon(const DesktopIcon& icon) const;

    bool m_dryRunInteractions = false;
    int m_interactableIconIndex = -1;
    std::wstring m_lastInteractableId;
    std::chrono::steady_clock::time_point m_lastInteractionTime {};
    bool m_hasPendingLaunch = false;
    std::wstring m_pendingLaunchPath;
    std::wstring m_pendingLaunchLabel;
    std::chrono::steady_clock::time_point m_pendingLaunchTime {};
    int m_bumpedIconIndex = -1;
    std::wstring m_lastBumpedIconId;
    std::chrono::steady_clock::time_point m_bumpHighlightUntil {};
};
