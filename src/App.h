#pragma once

#include "AppConfig.h"
#include "Character.h"
#include "DesktopIconService.h"
#include "DesktopWindow.h"
#include "Input.h"
#include "Renderer.h"

#include <chrono>
#include <vector>
#include <windows.h>

class App {
public:
    App(HINSTANCE instance, int showCommand, AppConfig config);

    int run();

private:
    bool initialize();
    void update(float deltaSeconds);
    void render();
    void refreshDesktopIcons();

    HINSTANCE m_instance = nullptr;
    int m_showCommand = SW_SHOWNORMAL;
    AppConfig m_config;
    DesktopWindow m_window;
    Renderer m_renderer;
    Input m_input;
    Character m_character;
    DesktopIconService m_desktopIconService;
    std::vector<DesktopIcon> m_desktopIcons;
    IconDebugOverlaySettings m_iconDebugOverlaySettings;
    std::chrono::steady_clock::time_point m_lastFrameTime;
};
