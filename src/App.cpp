#include "App.h"

#include "DebugLog.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <thread>

namespace {
constexpr float MAX_DELTA_SECONDS = 0.1f;
constexpr auto TARGET_FRAME_TIME = std::chrono::milliseconds(16);
constexpr bool ENABLE_ICON_COLLISION_EXPERIMENT = true;

D2D1_RECT_F screenRectToClientRect(const RECT& screenRect, POINT clientOrigin)
{
    return D2D1::RectF(
        static_cast<float>(screenRect.left - clientOrigin.x),
        static_cast<float>(screenRect.top - clientOrigin.y),
        static_cast<float>(screenRect.right - clientOrigin.x),
        static_cast<float>(screenRect.bottom - clientOrigin.y));
}
}

App::App(HINSTANCE instance, int showCommand, AppConfig config)
    : m_instance(instance),
      m_showCommand(showCommand),
      m_config(config),
      m_character({ 320.0f, 240.0f }),
      m_iconInteractionController(config.dryRunInteractions)
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
        m_iconDebugOverlaySettings.showOverlay = !m_iconDebugOverlaySettings.showOverlay;
        debugLog(m_iconDebugOverlaySettings.showOverlay ? L"Icon debug overlay enabled." : L"Icon debug overlay disabled.");
    }

    if (m_input.shouldToggleIconHoverBounds()) {
        m_iconDebugOverlaySettings.showHoverBounds = !m_iconDebugOverlaySettings.showHoverBounds;
        debugLog(m_iconDebugOverlaySettings.showHoverBounds ? L"Icon hover bounds enabled." : L"Icon hover bounds disabled.");
    }

    if (m_input.shouldToggleIconImageBounds()) {
        m_iconDebugOverlaySettings.showImageBounds = !m_iconDebugOverlaySettings.showImageBounds;
        debugLog(m_iconDebugOverlaySettings.showImageBounds ? L"Icon image bounds enabled." : L"Icon image bounds disabled.");
    }

    if (m_input.shouldToggleIconAnchors()) {
        m_iconDebugOverlaySettings.showAnchors = !m_iconDebugOverlaySettings.showAnchors;
        debugLog(m_iconDebugOverlaySettings.showAnchors ? L"Icon anchors enabled." : L"Icon anchors disabled.");
    }

    if (m_input.shouldToggleIconLabels()) {
        m_iconDebugOverlaySettings.showLabels = !m_iconDebugOverlaySettings.showLabels;
        debugLog(m_iconDebugOverlaySettings.showLabels ? L"Icon labels enabled." : L"Icon labels disabled.");
    }

    if (m_input.shouldRefreshDesktopIcons()) {
        refreshDesktopIcons();
    }

    if (m_input.shouldJump()) {
        m_character.startJump();
    }

    m_character.update(m_input.movementDirection(), deltaSeconds);

    const POINT clientOrigin = m_window.clientScreenOrigin();
    std::vector<D2D1_RECT_F> iconClientBounds;
    iconClientBounds.reserve(m_desktopIcons.size());
    for (const DesktopIcon& icon : m_desktopIcons) {
        iconClientBounds.push_back(screenRectToClientRect(icon.screenBounds, clientOrigin));
    }

    bool dashedThisFrame = false;
    const int dashDirection = m_input.consumeFastRollDashDirection();
    if (dashDirection != 0 && m_character.canDash()) {
        const Character::DashResult dashResult = m_character.tryFastRollDash(dashDirection, iconClientBounds);
        if (dashResult.dashed) {
            dashedThisFrame = true;
            debugLog(std::wstring(L"Fast-roll dash ")
                + (dashDirection < 0 ? L"left" : L"right")
                + L": ("
                + std::to_wstring(static_cast<int>(std::round(dashResult.startPosition.x)))
                + L", "
                + std::to_wstring(static_cast<int>(std::round(dashResult.startPosition.y)))
                + L") -> ("
                + std::to_wstring(static_cast<int>(std::round(dashResult.endPosition.x)))
                + L", "
                + std::to_wstring(static_cast<int>(std::round(dashResult.endPosition.y)))
                + L")");
        }
    }

    if (ENABLE_ICON_COLLISION_EXPERIMENT && !dashedThisFrame) {
        const bool strongCollision = m_character.isFastRolling();

        for (std::size_t index = 0; index < iconClientBounds.size(); ++index) {
            if (m_character.resolveIconCollision(iconClientBounds[index], strongCollision)) {
                m_iconInteractionController.noteIconCollision(
                    m_desktopIcons,
                    static_cast<int>(index),
                    strongCollision);
                break;
            }
        }
    }

    m_iconInteractionController.updateInteractableIcon(m_desktopIcons, characterScreenBounds());

    if (m_input.shouldInteract()
        && m_character.canStartInteraction()
        && m_iconInteractionController.tryInteract(
            m_desktopIcons,
            std::chrono::milliseconds(m_character.interactionLaunchDelayMilliseconds()))) {
        m_character.startInteraction();
    }

    m_iconInteractionController.updatePendingLaunch();
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
        m_iconDebugOverlaySettings,
        m_iconInteractionController.interactableIconIndex(),
        m_iconInteractionController.bumpedIconIndex(),
        m_input.isControlModeEnabled(),
        m_window.clientScreenOrigin());
}

void App::refreshDesktopIcons()
{
    m_desktopIcons = m_desktopIconService.refresh();
}

RECT App::characterScreenBounds() const
{
    const POINT clientOrigin = m_window.clientScreenOrigin();
    const D2D1_RECT_F bounds = m_character.bounds();

    return {
        clientOrigin.x + static_cast<LONG>(std::floor(bounds.left)),
        clientOrigin.y + static_cast<LONG>(std::floor(bounds.top)),
        clientOrigin.x + static_cast<LONG>(std::ceil(bounds.right)),
        clientOrigin.y + static_cast<LONG>(std::ceil(bounds.bottom))
    };
}
