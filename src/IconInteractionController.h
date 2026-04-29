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
    void tryInteract(const std::vector<DesktopIcon>& icons);
    int interactableIconIndex() const;

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
};
