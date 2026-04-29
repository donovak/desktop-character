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
    const bool isF3Down = isKeyDown(VK_F3);
    const bool isF4Down = isKeyDown(VK_F4);
    const bool isF5Down = isKeyDown(VK_F5);
    const bool isF6Down = isKeyDown(VK_F6);
    const bool isF7Down = isKeyDown(VK_F7);
    const bool isSpaceDown = isKeyDown(VK_SPACE);
    const bool isEDown = isKeyDown('E');
    m_shouldToggleIconDebugOverlay = isF2Down && !m_wasF2Down;
    m_shouldToggleIconHoverBounds = isF3Down && !m_wasF3Down;
    m_shouldToggleIconImageBounds = isF4Down && !m_wasF4Down;
    m_shouldRefreshDesktopIcons = isF5Down && !m_wasF5Down;
    m_shouldToggleIconAnchors = isF6Down && !m_wasF6Down;
    m_shouldToggleIconLabels = isF7Down && !m_wasF7Down;
    m_shouldInteract = (isSpaceDown && !m_wasSpaceDown) || (isEDown && !m_wasEDown);
    m_wasF2Down = isF2Down;
    m_wasF3Down = isF3Down;
    m_wasF4Down = isF4Down;
    m_wasF5Down = isF5Down;
    m_wasF6Down = isF6Down;
    m_wasF7Down = isF7Down;
    m_wasSpaceDown = isSpaceDown;
    m_wasEDown = isEDown;

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

bool Input::shouldToggleIconHoverBounds() const
{
    return m_shouldToggleIconHoverBounds;
}

bool Input::shouldToggleIconImageBounds() const
{
    return m_shouldToggleIconImageBounds;
}

bool Input::shouldToggleIconAnchors() const
{
    return m_shouldToggleIconAnchors;
}

bool Input::shouldToggleIconLabels() const
{
    return m_shouldToggleIconLabels;
}

bool Input::shouldRefreshDesktopIcons() const
{
    return m_shouldRefreshDesktopIcons;
}

bool Input::shouldInteract() const
{
    return m_shouldInteract;
}
