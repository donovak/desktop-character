#pragma once

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

class Input {
public:
    void update();
    Vector2 movementDirection() const;
    bool shouldExit() const;
    bool shouldToggleIconDebugOverlay() const;
    bool shouldRefreshDesktopIcons() const;

private:
    Vector2 m_movementDirection {};
    bool m_shouldExit = false;
    bool m_shouldToggleIconDebugOverlay = false;
    bool m_shouldRefreshDesktopIcons = false;
    bool m_wasF2Down = false;
    bool m_wasF5Down = false;
};
