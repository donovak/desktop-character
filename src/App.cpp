#include "App.h"

#include <algorithm>
#include <thread>

namespace {
constexpr float MAX_DELTA_SECONDS = 0.1f;
constexpr auto TARGET_FRAME_TIME = std::chrono::milliseconds(16);
}

App::App(HINSTANCE instance, int showCommand)
    : m_instance(instance),
      m_showCommand(showCommand),
      m_character({ 320.0f, 240.0f })
{
}

int App::run()
{
    if (!initialize()) {
        return -1;
    }

    m_lastFrameTime = std::chrono::steady_clock::now();

    MSG message {};
    while (m_window.isRunning()) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                m_window.requestClose();
                break;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!m_window.isRunning()) {
            break;
        }

        const auto frameStart = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsed = frameStart - m_lastFrameTime;
        m_lastFrameTime = frameStart;

        update(std::min(elapsed.count(), MAX_DELTA_SECONDS));
        render();

        const auto frameDuration = std::chrono::steady_clock::now() - frameStart;
        if (frameDuration < TARGET_FRAME_TIME) {
            std::this_thread::sleep_for(TARGET_FRAME_TIME - frameDuration);
        }
    }

    return static_cast<int>(message.wParam);
}

bool App::initialize()
{
    if (!m_window.create(m_instance, m_showCommand)) {
        return false;
    }

    const RECT clientRect = m_window.clientRect();
    m_character.setBounds(
        static_cast<float>(clientRect.right - clientRect.left),
        static_cast<float>(clientRect.bottom - clientRect.top));

    if (!m_renderer.initialize(m_window.handle())) {
        return false;
    }

    return true;
}

void App::update(float deltaSeconds)
{
    const RECT clientRect = m_window.clientRect();
    m_character.setBounds(
        static_cast<float>(clientRect.right - clientRect.left),
        static_cast<float>(clientRect.bottom - clientRect.top));

    m_input.update();
    m_character.update(m_input.movementDirection(), deltaSeconds);
}

void App::render()
{
    const RECT clientRect = m_window.clientRect();
    m_renderer.resizeIfNeeded(
        static_cast<unsigned int>(clientRect.right - clientRect.left),
        static_cast<unsigned int>(clientRect.bottom - clientRect.top));

    m_renderer.render(m_character);
}
