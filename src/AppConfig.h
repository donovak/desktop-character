#pragma once

#include <string_view>

enum class WindowMode {
    NormalWindow,
    DesktopOverlay
};

struct AppConfig {
    WindowMode windowMode = WindowMode::DesktopOverlay;
    bool transparentOverlayBackground = true;
    bool enableClickThrough = false;
    bool dryRunInteractions = false;

    static AppConfig fromCommandLine(std::wstring_view commandLine);
};

constexpr unsigned char OVERLAY_TRANSPARENT_KEY_R = 2;
constexpr unsigned char OVERLAY_TRANSPARENT_KEY_G = 3;
constexpr unsigned char OVERLAY_TRANSPARENT_KEY_B = 5;
