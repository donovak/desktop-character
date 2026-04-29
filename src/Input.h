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

private:
    Vector2 m_movementDirection {};
    bool m_shouldExit = false;
};
