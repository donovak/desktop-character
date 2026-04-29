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
    bool shouldToggleIconHoverBounds() const;
    bool shouldToggleIconImageBounds() const;
    bool shouldToggleIconAnchors() const;
    bool shouldToggleIconLabels() const;
    bool shouldRefreshDesktopIcons() const;

private:
    Vector2 m_movementDirection {};
    bool m_shouldExit = false;
    bool m_shouldToggleIconDebugOverlay = false;
    bool m_shouldToggleIconHoverBounds = false;
    bool m_shouldToggleIconImageBounds = false;
    bool m_shouldToggleIconAnchors = false;
    bool m_shouldToggleIconLabels = false;
    bool m_shouldRefreshDesktopIcons = false;
    bool m_wasF2Down = false;
    bool m_wasF3Down = false;
    bool m_wasF4Down = false;
    bool m_wasF5Down = false;
    bool m_wasF6Down = false;
    bool m_wasF7Down = false;
};
