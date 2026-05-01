#pragma once

#include <chrono>

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

class Input {
public:
    Input();
    ~Input();

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    void update();
    Vector2 movementDirection() const;
    bool shouldExit() const;
    bool isControlModeEnabled() const;
    bool shouldToggleIconDebugOverlay() const;
    bool shouldToggleIconHoverBounds() const;
    bool shouldToggleIconImageBounds() const;
    bool shouldToggleIconAnchors() const;
    bool shouldToggleIconLabels() const;
    bool shouldRefreshDesktopIcons() const;
    bool shouldInteract() const;
    bool shouldJump() const;
    int consumeFastRollDashDirection();

private:
    void installKeyboardHook();
    void uninstallKeyboardHook();
    void updateTapDash(bool isLeftDown, bool isRightDown);
    void setControlModeEnabled(bool enabled);

    Vector2 m_movementDirection {};
    bool m_shouldExit = false;
    bool m_controlModeEnabled = false;
    bool m_shouldToggleIconDebugOverlay = false;
    bool m_shouldToggleIconHoverBounds = false;
    bool m_shouldToggleIconImageBounds = false;
    bool m_shouldToggleIconAnchors = false;
    bool m_shouldToggleIconLabels = false;
    bool m_shouldRefreshDesktopIcons = false;
    bool m_shouldInteract = false;
    bool m_shouldJump = false;
    int m_pendingFastRollDashDirection = 0;
    bool m_wasF2Down = false;
    bool m_wasF3Down = false;
    bool m_wasF4Down = false;
    bool m_wasF5Down = false;
    bool m_wasF6Down = false;
    bool m_wasF7Down = false;
    bool m_wasF8Down = false;
    bool m_wasSpaceDown = false;
    bool m_wasEDown = false;
    bool m_wasLeftDashDown = false;
    bool m_wasRightDashDown = false;
    std::chrono::steady_clock::time_point m_lastLeftTapTime {};
    std::chrono::steady_clock::time_point m_lastRightTapTime {};
};
