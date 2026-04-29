#pragma once

#include "Input.h"

#include <d2d1.h>

class Character {
public:
    explicit Character(Vector2 position);

    void update(Vector2 movementDirection, float deltaSeconds);
    void setBounds(float width, float height);
    D2D1_RECT_F bounds() const;

private:
    Vector2 normalized(Vector2 value) const;

    Vector2 m_position {};
    float m_speed = 260.0f;
    float m_size = 48.0f;
    float m_boundsWidth = 0.0f;
    float m_boundsHeight = 0.0f;
};
