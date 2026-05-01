#include "Character.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float MOVING_SPEED_THRESHOLD = 8.0f;
constexpr float FACING_SPEED_THRESHOLD = 4.0f;
constexpr float WALK_MAX_SPEED = 235.0f;
constexpr float FAST_ROLL_SPEED_THRESHOLD = 220.0f;
constexpr float FAST_ROLL_MAX_SPEED = 345.0f;
constexpr float ACCELERATION = 520.0f;
constexpr float DECELERATION = 360.0f;
constexpr float FAST_ROLL_ENTRY_TIME = 1.15f;
constexpr float SAME_DIRECTION_DOT_THRESHOLD = 0.86f;
constexpr float SPRITE_CELL_WIDTH = 180.0f;
constexpr float SPRITE_CELL_HEIGHT = 160.0f;
constexpr float FAST_ROLL_END_HOLD_MULTIPLIER = 4.0f;
constexpr float REGULAR_COLLISION_DAMPING = 0.25f;
constexpr float FAST_COLLISION_BOUNCE = 0.42f;
constexpr float FAST_COLLISION_TANGENTIAL_DAMPING = 0.72f;
constexpr float FAST_ROLL_DASH_DISTANCE = 220.0f;
constexpr float FAST_ROLL_DASH_MAX_EXTRA_DISTANCE = 260.0f;
constexpr float DASH_ICON_ESCAPE_STEP = 12.0f;
constexpr float DASH_VISUAL_DURATION = 0.18f;

constexpr Character::AnimationDefinition IDLE_ANIMATION {
    0,
    0,
    8,
    0.32f,
    true
};

constexpr Character::AnimationDefinition ROLL_START_ANIMATION {
    1,
    0,
    3,
    0.13f,
    false
};

constexpr Character::AnimationDefinition ROLL_LOOP_ANIMATION {
    1,
    4,
    6,
    0.14f,
    true
};

constexpr Character::AnimationDefinition ROLL_END_ANIMATION {
    1,
    7,
    9,
    0.18f,
    false
};

constexpr Character::AnimationDefinition FAST_ROLL_START_ANIMATION {
    2,
    0,
    1,
    0.12f,
    false
};

constexpr Character::AnimationDefinition FAST_ROLL_LOOP_ANIMATION {
    2,
    2,
    6,
    0.10f,
    true
};

constexpr Character::AnimationDefinition FAST_ROLL_END_ANIMATION {
    2,
    7,
    7,
    0.18f,
    false
};

constexpr Character::AnimationDefinition JUMP_ANIMATION {
    3,
    0,
    8,
    0.12f,
    false
};

constexpr Character::AnimationDefinition INTERACT_ANIMATION {
    4,
    0,
    8,
    0.13f,
    false
};

float moveTowards(float current, float target, float maxDelta)
{
    if (std::abs(target - current) <= maxDelta) {
        return target;
    }

    return current + std::copysign(maxDelta, target - current);
}
}

Character::Character(Vector2 position)
    : m_position(position)
{
}

void Character::update(Vector2 movementDirection, float deltaSeconds)
{
    m_dashVisualSecondsRemaining = std::max(0.0f, m_dashVisualSecondsRemaining - deltaSeconds);
    updateMovement(movementDirection, deltaSeconds);
    updateAnimation(deltaSeconds);
}

void Character::startInteraction()
{
    enterState(CharacterAnimationState::Interact);
}

void Character::startJump()
{
    if (m_animationState == CharacterAnimationState::Interact) {
        return;
    }

    enterState(CharacterAnimationState::Jump);
}

void Character::updateMovement(Vector2 movementDirection, float deltaSeconds)
{
    const Vector2 direction = normalized(movementDirection);
    m_hasMovementInput = direction.x != 0.0f || direction.y != 0.0f;

    if (m_hasMovementInput) {
        const float dot = (m_lastMoveDirection.x * direction.x) + (m_lastMoveDirection.y * direction.y);
        if (dot >= SAME_DIRECTION_DOT_THRESHOLD) {
            m_sameDirectionMoveSeconds += deltaSeconds;
        } else {
            m_sameDirectionMoveSeconds = 0.0f;
        }

        m_lastMoveDirection = direction;
    } else {
        m_sameDirectionMoveSeconds = 0.0f;
        m_lastMoveDirection = {};
    }

    const Vector2 targetVelocity {
        direction.x * (isFastRolling() ? FAST_ROLL_MAX_SPEED : WALK_MAX_SPEED),
        direction.y * (isFastRolling() ? FAST_ROLL_MAX_SPEED : WALK_MAX_SPEED)
    };

    const float maxDelta = (m_hasMovementInput ? ACCELERATION : DECELERATION) * deltaSeconds;
    m_velocity.x = moveTowards(m_velocity.x, targetVelocity.x, maxDelta);
    m_velocity.y = moveTowards(m_velocity.y, targetVelocity.y, maxDelta);

    if (m_velocity.x < -FACING_SPEED_THRESHOLD) {
        m_facingRight = false;
    } else if (m_velocity.x > FACING_SPEED_THRESHOLD) {
        m_facingRight = true;
    }

    m_position.x += m_velocity.x * deltaSeconds;
    m_position.y += m_velocity.y * deltaSeconds;

    if (m_boundsWidth > 0.0f && m_boundsHeight > 0.0f) {
        m_position = clampedPosition(m_position);
    }
}

void Character::setBounds(float width, float height)
{
    m_boundsWidth = width;
    m_boundsHeight = height;
}

D2D1_RECT_F Character::bounds() const
{
    return boundsAtPosition(m_position);
}

D2D1_RECT_F Character::boundsAtPosition(Vector2 position) const
{
    const float halfWidth = m_collisionWidth * 0.5f;
    const float halfHeight = m_collisionHeight * 0.5f;
    return D2D1::RectF(
        position.x - halfWidth,
        position.y - halfHeight,
        position.x + halfWidth,
        position.y + halfHeight);
}

D2D1_RECT_F Character::spriteDestinationBounds() const
{
    const float halfWidth = m_spriteDrawWidth * 0.5f;
    const float halfHeight = m_spriteDrawHeight * 0.5f;
    return D2D1::RectF(
        m_position.x - halfWidth,
        m_position.y - halfHeight,
        m_position.x + halfWidth,
        m_position.y + halfHeight);
}

D2D1_RECT_F Character::spriteSourceRect() const
{
    const AnimationDefinition& animation = currentAnimation();
    return D2D1::RectF(
        static_cast<float>(m_currentFrame) * SPRITE_CELL_WIDTH,
        static_cast<float>(animation.row) * SPRITE_CELL_HEIGHT,
        (static_cast<float>(m_currentFrame) + 1.0f) * SPRITE_CELL_WIDTH,
        (static_cast<float>(animation.row) + 1.0f) * SPRITE_CELL_HEIGHT);
}

bool Character::isFacingRight() const
{
    return m_facingRight;
}

bool Character::isFastRolling() const
{
    return m_animationState == CharacterAnimationState::FastRollStart
        || m_animationState == CharacterAnimationState::FastRollLoop;
}

bool Character::canDash() const
{
    switch (m_animationState) {
    case CharacterAnimationState::RollStart:
    case CharacterAnimationState::RollLoop:
    case CharacterAnimationState::RollEnd:
    case CharacterAnimationState::FastRollStart:
    case CharacterAnimationState::FastRollLoop:
    case CharacterAnimationState::FastRollEnd:
        return isMoving();
    case CharacterAnimationState::Idle:
    case CharacterAnimationState::Jump:
    case CharacterAnimationState::Interact:
    default:
        return false;
    }
}

bool Character::isDashVisualActive() const
{
    return m_dashVisualSecondsRemaining > 0.0f;
}

Vector2 Character::dashStartPosition() const
{
    return m_dashStartPosition;
}

Vector2 Character::dashEndPosition() const
{
    return m_dashEndPosition;
}

bool Character::canStartInteraction() const
{
    return m_animationState != CharacterAnimationState::Interact;
}

int Character::interactionLaunchDelayMilliseconds() const
{
    const float secondsToFrameThree = (INTERACT_ANIMATION.frameDurationSeconds * 2.0f)
        + (INTERACT_ANIMATION.frameDurationSeconds * 2.8f);
    return static_cast<int>(secondsToFrameThree * 1000.0f);
}

CharacterAnimationState Character::animationState() const
{
    return m_animationState;
}

bool Character::resolveIconCollision(D2D1_RECT_F iconBounds, bool strongResponse)
{
    const D2D1_RECT_F characterBounds = bounds();
    if (characterBounds.left >= iconBounds.right
        || characterBounds.right <= iconBounds.left
        || characterBounds.top >= iconBounds.bottom
        || characterBounds.bottom <= iconBounds.top) {
        return false;
    }

    const float pushLeft = characterBounds.right - iconBounds.left;
    const float pushRight = iconBounds.right - characterBounds.left;
    const float pushUp = characterBounds.bottom - iconBounds.top;
    const float pushDown = iconBounds.bottom - characterBounds.top;
    const float resolveX = pushLeft < pushRight ? -pushLeft : pushRight;
    const float resolveY = pushUp < pushDown ? -pushUp : pushDown;

    if (std::abs(resolveX) < std::abs(resolveY)) {
        m_position.x += resolveX;
        if (strongResponse) {
            m_velocity.x = -m_velocity.x * FAST_COLLISION_BOUNCE;
            m_velocity.y *= FAST_COLLISION_TANGENTIAL_DAMPING;
        } else if ((resolveX < 0.0f && m_velocity.x > 0.0f) || (resolveX > 0.0f && m_velocity.x < 0.0f)) {
            m_velocity.x = 0.0f;
            m_velocity.y *= REGULAR_COLLISION_DAMPING;
        }
    } else {
        m_position.y += resolveY;
        if (strongResponse) {
            m_velocity.y = -m_velocity.y * FAST_COLLISION_BOUNCE;
            m_velocity.x *= FAST_COLLISION_TANGENTIAL_DAMPING;
        } else if ((resolveY < 0.0f && m_velocity.y > 0.0f) || (resolveY > 0.0f && m_velocity.y < 0.0f)) {
            m_velocity.y = 0.0f;
            m_velocity.x *= REGULAR_COLLISION_DAMPING;
        }
    }

    return true;
}

Character::DashResult Character::tryFastRollDash(int direction, const std::vector<D2D1_RECT_F>& iconBounds)
{
    if (direction == 0 || !canDash()) {
        return {};
    }

    const float dashDirection = direction < 0 ? -1.0f : 1.0f;
    const Vector2 startPosition = m_position;
    Vector2 targetPosition {
        m_position.x + (dashDirection * FAST_ROLL_DASH_DISTANCE),
        m_position.y
    };

    targetPosition = clampedPosition(targetPosition);

    float extraDistance = 0.0f;
    while (overlapsAnyIcon(boundsAtPosition(targetPosition), iconBounds)
        && extraDistance < FAST_ROLL_DASH_MAX_EXTRA_DISTANCE) {
        const float previousX = targetPosition.x;
        targetPosition.x += dashDirection * DASH_ICON_ESCAPE_STEP;
        targetPosition = clampedPosition(targetPosition);
        extraDistance += std::abs(targetPosition.x - previousX);

        if (targetPosition.x == previousX) {
            break;
        }
    }

    m_position = targetPosition;
    m_velocity.x = dashDirection * std::max(std::abs(m_velocity.x), FAST_ROLL_SPEED_THRESHOLD);
    m_lastMoveDirection = { dashDirection, 0.0f };
    m_sameDirectionMoveSeconds = std::max(m_sameDirectionMoveSeconds, FAST_ROLL_ENTRY_TIME);
    if (m_animationState == CharacterAnimationState::RollEnd
        || m_animationState == CharacterAnimationState::FastRollEnd) {
        enterState(CharacterAnimationState::FastRollLoop);
    }

    m_dashVisualSecondsRemaining = DASH_VISUAL_DURATION;
    m_dashStartPosition = startPosition;
    m_dashEndPosition = targetPosition;

    return {
        true,
        startPosition,
        targetPosition
    };
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

Vector2 Character::clampedPosition(Vector2 position) const
{
    if (m_boundsWidth <= 0.0f || m_boundsHeight <= 0.0f) {
        return position;
    }

    const float halfWidth = m_collisionWidth * 0.5f;
    const float halfHeight = m_collisionHeight * 0.5f;
    const float minX = halfWidth;
    const float minY = halfHeight;
    const float maxX = std::max(minX, m_boundsWidth - halfWidth);
    const float maxY = std::max(minY, m_boundsHeight - halfHeight);

    return {
        std::clamp(position.x, minX, maxX),
        std::clamp(position.y, minY, maxY)
    };
}

bool Character::overlapsAnyIcon(D2D1_RECT_F characterBounds, const std::vector<D2D1_RECT_F>& iconBounds) const
{
    for (const D2D1_RECT_F& icon : iconBounds) {
        if (characterBounds.left < icon.right
            && characterBounds.right > icon.left
            && characterBounds.top < icon.bottom
            && characterBounds.bottom > icon.top) {
            return true;
        }
    }

    return false;
}

void Character::updateAnimation(float deltaSeconds)
{
    switch (m_animationState) {
    case CharacterAnimationState::Idle:
        if (isMoving()) {
            enterState(CharacterAnimationState::RollStart);
        }
        break;
    case CharacterAnimationState::RollLoop:
        if (!isMoving()) {
            enterState(CharacterAnimationState::RollEnd);
        } else if (shouldFastRoll()) {
            enterState(CharacterAnimationState::FastRollStart);
        }
        break;
    case CharacterAnimationState::FastRollLoop:
        if (!m_hasMovementInput && isMoving()) {
            enterState(CharacterAnimationState::FastRollEnd);
        } else if (!shouldFastRoll()) {
            enterState(CharacterAnimationState::FastRollEnd);
        }
        break;
    default:
        break;
    }

    const AnimationDefinition& animation = currentAnimation();
    m_frameElapsedSeconds += deltaSeconds;

    while (m_frameElapsedSeconds >= currentFrameDuration()) {
        m_frameElapsedSeconds -= currentFrameDuration();
        ++m_currentFrame;

        if (m_currentFrame <= animation.lastFrame) {
            continue;
        }

        if (animation.loops) {
            m_currentFrame = animation.firstFrame;
        } else {
            completeCurrentAnimation();
            break;
        }
    }

    const AnimationDefinition& current = currentAnimation();
    if (m_currentFrame < current.firstFrame || m_currentFrame > current.lastFrame) {
        m_currentFrame = current.firstFrame;
    }
}

void Character::enterState(CharacterAnimationState state)
{
    m_animationState = state;
    m_currentFrame = currentAnimation().firstFrame;
    m_frameElapsedSeconds = 0.0f;
}

void Character::completeCurrentAnimation()
{
    switch (m_animationState) {
    case CharacterAnimationState::RollStart:
        enterState(shouldFastRoll() ? CharacterAnimationState::FastRollStart
                                    : (isMoving() ? CharacterAnimationState::RollLoop : CharacterAnimationState::RollEnd));
        break;
    case CharacterAnimationState::RollEnd:
        enterState(isMoving() ? CharacterAnimationState::RollStart : CharacterAnimationState::Idle);
        break;
    case CharacterAnimationState::FastRollStart:
        enterState(shouldFastRoll() ? CharacterAnimationState::FastRollLoop : CharacterAnimationState::FastRollEnd);
        break;
    case CharacterAnimationState::FastRollEnd:
        enterState(isMoving() ? CharacterAnimationState::RollEnd : CharacterAnimationState::Idle);
        break;
    case CharacterAnimationState::Jump:
    case CharacterAnimationState::Interact:
        enterState(shouldFastRoll() ? CharacterAnimationState::FastRollLoop
                                    : (isMoving() ? CharacterAnimationState::RollLoop : CharacterAnimationState::Idle));
        break;
    default:
        enterState(isMoving() ? CharacterAnimationState::RollStart : CharacterAnimationState::Idle);
        break;
    }
}

const Character::AnimationDefinition& Character::currentAnimation() const
{
    switch (m_animationState) {
    case CharacterAnimationState::RollStart:
        return ROLL_START_ANIMATION;
    case CharacterAnimationState::RollLoop:
        return ROLL_LOOP_ANIMATION;
    case CharacterAnimationState::RollEnd:
        return ROLL_END_ANIMATION;
    case CharacterAnimationState::FastRollStart:
        return FAST_ROLL_START_ANIMATION;
    case CharacterAnimationState::FastRollLoop:
        return FAST_ROLL_LOOP_ANIMATION;
    case CharacterAnimationState::FastRollEnd:
        return FAST_ROLL_END_ANIMATION;
    case CharacterAnimationState::Jump:
        return JUMP_ANIMATION;
    case CharacterAnimationState::Interact:
        return INTERACT_ANIMATION;
    case CharacterAnimationState::Idle:
    default:
        return IDLE_ANIMATION;
    }
}

float Character::currentFrameDuration() const
{
    const AnimationDefinition& animation = currentAnimation();

    if (m_animationState == CharacterAnimationState::Interact && m_currentFrame == 2) {
        return animation.frameDurationSeconds * 2.8f;
    }

    if (m_animationState == CharacterAnimationState::FastRollEnd && m_currentFrame == 7) {
        return animation.frameDurationSeconds * FAST_ROLL_END_HOLD_MULTIPLIER;
    }

    return animation.frameDurationSeconds;
}

bool Character::shouldFastRoll() const
{
    if (!m_hasMovementInput || !isMoving()) {
        return false;
    }

    return speed() >= FAST_ROLL_SPEED_THRESHOLD
        && m_sameDirectionMoveSeconds >= FAST_ROLL_ENTRY_TIME;
}

bool Character::isMoving() const
{
    return speed() > MOVING_SPEED_THRESHOLD;
}

float Character::speed() const
{
    return std::sqrt((m_velocity.x * m_velocity.x) + (m_velocity.y * m_velocity.y));
}
