#include "Character.h"

#include <algorithm>
#include <cmath>

Character::Character(Vector2 position)
    : m_position(position)
{
}

void Character::update(Vector2 movementDirection, float deltaSeconds)
{
    const Vector2 direction = normalized(movementDirection);
    m_position.x += direction.x * m_speed * deltaSeconds;
    m_position.y += direction.y * m_speed * deltaSeconds;

    if (m_boundsWidth > 0.0f && m_boundsHeight > 0.0f) {
        const float maxX = std::max(0.0f, m_boundsWidth - m_size);
        const float maxY = std::max(0.0f, m_boundsHeight - m_size);
        m_position.x = std::clamp(m_position.x, 0.0f, maxX);
        m_position.y = std::clamp(m_position.y, 0.0f, maxY);
    }
}

void Character::setBounds(float width, float height)
{
    m_boundsWidth = width;
    m_boundsHeight = height;
}

D2D1_RECT_F Character::bounds() const
{
    return D2D1::RectF(
        m_position.x,
        m_position.y,
        m_position.x + m_size,
        m_position.y + m_size);
}

Vector2 Character::normalized(Vector2 value) const
{
    const float lengthSquared = value.x * value.x + value.y * value.y;
    if (lengthSquared <= 0.0f) {
        return {};
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return { value.x * inverseLength, value.y * inverseLength };
}
