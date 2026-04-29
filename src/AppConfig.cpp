#include "AppConfig.h"

AppConfig AppConfig::fromCommandLine(std::wstring_view commandLine)
{
    AppConfig config {};

    if (commandLine.find(L"--normal-window") != std::wstring_view::npos) {
        config.windowMode = WindowMode::NormalWindow;
    }

    if (commandLine.find(L"--desktop-overlay") != std::wstring_view::npos) {
        config.windowMode = WindowMode::DesktopOverlay;
    }

    if (commandLine.find(L"--opaque-overlay") != std::wstring_view::npos) {
        config.transparentOverlayBackground = false;
    }

    if (commandLine.find(L"--enable-click-through") != std::wstring_view::npos) {
        config.enableClickThrough = true;
    }

    return config;
}
