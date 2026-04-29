#include "App.h"

#include "DebugLog.h"

#include <algorithm>
#include <thread>

namespace {
constexpr float MAX_DELTA_SECONDS = 0.1f;
constexpr auto TARGET_FRAME_TIME = std::chrono::milliseconds(16);
}

App::App(HINSTANCE instance, int showCommand, AppConfig config)
    : m_instance(instance),
      m_showCommand(showCommand),
      m_config(config),
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
        if (!m_window.isRunning()) {
            break;
        }

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
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        debugLog(L"SetProcessDpiAwarenessContext failed or was already fixed by the process.");
    }

    if (!m_window.create(m_instance, m_showCommand, m_config)) {
        return false;
    }

    const RECT clientRect = m_window.clientRect();
    m_character.setBounds(
        static_cast<float>(clientRect.right - clientRect.left),
        static_cast<float>(clientRect.bottom - clientRect.top));

    const bool useTransparentBackground =
        m_config.windowMode == WindowMode::DesktopOverlay && m_config.transparentOverlayBackground;

    if (!m_renderer.initialize(m_window.handle(), useTransparentBackground)) {
        return false;
    }

    refreshDesktopIcons();
    return true;
}

void App::update(float deltaSeconds)
{
    const RECT clientRect = m_window.clientRect();
    m_character.setBounds(
        static_cast<float>(clientRect.right - clientRect.left),
        static_cast<float>(clientRect.bottom - clientRect.top));

    m_input.update();
    if (m_input.shouldExit()) {
        m_window.requestClose();
        return;
    }

    if (m_input.shouldToggleIconDebugOverlay()) {
        m_showIconDebugOverlay = !m_showIconDebugOverlay;
        debugLog(m_showIconDebugOverlay ? L"Icon debug overlay enabled." : L"Icon debug overlay disabled.");
    }

    if (m_input.shouldRefreshDesktopIcons()) {
        refreshDesktopIcons();
    }

    m_character.update(m_input.movementDirection(), deltaSeconds);
}

void App::render()
{
    const RECT clientRect = m_window.clientRect();
    m_renderer.resizeIfNeeded(
        static_cast<unsigned int>(clientRect.right - clientRect.left),
        static_cast<unsigned int>(clientRect.bottom - clientRect.top));

    m_renderer.render(
        m_character,
        m_desktopIcons,
        m_showIconDebugOverlay,
        m_window.clientScreenOrigin());
}

void App::refreshDesktopIcons()
{
    m_desktopIcons = m_desktopIconService.refresh();
}
