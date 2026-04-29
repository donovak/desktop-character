#pragma once

#include "Character.h"
#include "DesktopWindow.h"
#include "Input.h"
#include "Renderer.h"

#include <chrono>
#include <windows.h>

class App {
public:
    App(HINSTANCE instance, int showCommand);

    int run();

private:
    bool initialize();
    void update(float deltaSeconds);
    void render();

    HINSTANCE m_instance = nullptr;
    int m_showCommand = SW_SHOWNORMAL;
    DesktopWindow m_window;
    Renderer m_renderer;
    Input m_input;
    Character m_character;
    std::chrono::steady_clock::time_point m_lastFrameTime;
};
