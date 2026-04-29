#include "Input.h"

#include <windows.h>

namespace {
bool isKeyDown(int virtualKey)
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}
}

void Input::update()
{
    m_movementDirection = {};
    m_shouldExit = isKeyDown(VK_ESCAPE);

    if (isKeyDown('A') || isKeyDown(VK_LEFT)) {
        m_movementDirection.x -= 1.0f;
    }

    if (isKeyDown('D') || isKeyDown(VK_RIGHT)) {
        m_movementDirection.x += 1.0f;
    }

    if (isKeyDown('W') || isKeyDown(VK_UP)) {
        m_movementDirection.y -= 1.0f;
    }

    if (isKeyDown('S') || isKeyDown(VK_DOWN)) {
        m_movementDirection.y += 1.0f;
    }
}

Vector2 Input::movementDirection() const
{
    return m_movementDirection;
}

bool Input::shouldExit() const
{
    return m_shouldExit;
}
