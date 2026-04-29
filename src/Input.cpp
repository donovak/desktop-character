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

    const bool isF2Down = isKeyDown(VK_F2);
    const bool isF5Down = isKeyDown(VK_F5);
    m_shouldToggleIconDebugOverlay = isF2Down && !m_wasF2Down;
    m_shouldRefreshDesktopIcons = isF5Down && !m_wasF5Down;
    m_wasF2Down = isF2Down;
    m_wasF5Down = isF5Down;

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

bool Input::shouldToggleIconDebugOverlay() const
{
    return m_shouldToggleIconDebugOverlay;
}

bool Input::shouldRefreshDesktopIcons() const
{
    return m_shouldRefreshDesktopIcons;
}
