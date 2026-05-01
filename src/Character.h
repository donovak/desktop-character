#pragma once

#include "Input.h"

#include <d2d1.h>
#include <vector>

enum class CharacterAnimationState {
    Idle,
    RollStart,
    RollLoop,
    RollEnd,
    FastRollStart,
    FastRollLoop,
    FastRollEnd,
    Jump,
    Interact
};

class Character {
public:
    struct AnimationDefinition {
        int row = 0;
        int firstFrame = 0;
        int lastFrame = 0;
        float frameDurationSeconds = 0.1f;
        bool loops = true;
    };

    struct DashResult {
        bool dashed = false;
        Vector2 startPosition {};
        Vector2 endPosition {};
    };

    explicit Character(Vector2 position);

    void update(Vector2 movementDirection, float deltaSeconds);
    void startInteraction();
    void startJump();
    void setBounds(float width, float height);
    D2D1_RECT_F bounds() const;
    D2D1_RECT_F spriteDestinationBounds() const;
    D2D1_RECT_F spriteSourceRect() const;
    bool isFacingRight() const;
    bool isFastRolling() const;
    bool canDash() const;
    bool isDashVisualActive() const;
    Vector2 dashStartPosition() const;
    Vector2 dashEndPosition() const;
    bool canStartInteraction() const;
    bool resolveIconCollision(D2D1_RECT_F iconBounds, bool strongResponse);
    DashResult tryFastRollDash(int direction, const std::vector<D2D1_RECT_F>& iconBounds);
    int interactionLaunchDelayMilliseconds() const;
    CharacterAnimationState animationState() const;

private:
    Vector2 normalized(Vector2 value) const;
    void updateMovement(Vector2 movementDirection, float deltaSeconds);
    void updateAnimation(float deltaSeconds);
    void enterState(CharacterAnimationState state);
    void completeCurrentAnimation();
    D2D1_RECT_F boundsAtPosition(Vector2 position) const;
    Vector2 clampedPosition(Vector2 position) const;
    bool overlapsAnyIcon(D2D1_RECT_F characterBounds, const std::vector<D2D1_RECT_F>& iconBounds) const;
    const AnimationDefinition& currentAnimation() const;
    float currentFrameDuration() const;
    bool shouldFastRoll() const;
    bool isMoving() const;
    float speed() const;

    Vector2 m_position {};
    Vector2 m_velocity {};
    CharacterAnimationState m_animationState = CharacterAnimationState::Idle;
    int m_currentFrame = 0;
    float m_frameElapsedSeconds = 0.0f;
    Vector2 m_lastMoveDirection {};
    float m_sameDirectionMoveSeconds = 0.0f;
    bool m_hasMovementInput = false;
    bool m_facingRight = true;
    float m_collisionWidth = 48.0f;
    float m_collisionHeight = 48.0f;
    float m_spriteDrawWidth = 96.0f;
    float m_spriteDrawHeight = 86.0f;
    float m_boundsWidth = 0.0f;
    float m_boundsHeight = 0.0f;
    float m_dashVisualSecondsRemaining = 0.0f;
    Vector2 m_dashStartPosition {};
    Vector2 m_dashEndPosition {};
};
