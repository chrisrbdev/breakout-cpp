#include "core/BreakoutGame.h"

#include "core/BrickFactory.h"
#include "rendering/GameRenderer.h"
#include "raylib.h"

namespace breakout {

BreakoutGame::BreakoutGame() {
    InitWindow(config_.screenWidth, config_.screenHeight, "cpp-breakout-raylib");
    SetTargetFPS(config_.targetFps);
    ResetGame();
}

BreakoutGame::~BreakoutGame() {
    CloseWindow();
}

void BreakoutGame::Run() {
    while (!WindowShouldClose()) {
        Update();
        Render();
    }
}

void BreakoutGame::ResetGame() {
    session_.score = 0;
    session_.lives = config_.initialLives;
    session_.bricks = CreateBricks(config_.brickLayout);
    ResetRound();
    session_.state = GameState::Start;
}

void BreakoutGame::ResetRound() {
    session_.paddle.width = config_.paddleWidth;
    session_.paddle.height = config_.paddleHeight;
    session_.paddle.x = (config_.screenWidth - session_.paddle.width) / 2.0f;
    session_.paddle.y = config_.screenHeight - config_.paddleBottomOffset;
    session_.ballSpeed = {0.0f, config_.ballLaunchSpeed};
    SyncBallWithPaddle();
}

void BreakoutGame::Update() {
    switch (session_.state) {
        case GameState::Start:
            UpdateStartState();
            break;
        case GameState::Playing:
            UpdatePlayingState();
            break;
        case GameState::Win:
        case GameState::GameOver:
            UpdateEndState();
            break;
    }
}

void BreakoutGame::UpdateStartState() {
    UpdatePaddleMovement();
    SyncBallWithPaddle();

    if (IsKeyPressed(KEY_SPACE)) {
        session_.ballSpeed = {0.0f, config_.ballLaunchSpeed};
        session_.state = GameState::Playing;
    }
}

void BreakoutGame::UpdatePlayingState() {
    UpdatePaddleMovement();
    UpdateBallPosition();
    HandleWallCollision();
    HandlePaddleCollision();
    HandleBrickCollision();
    HandleLifeLoss();

    if (session_.state == GameState::Playing && AreAllBricksDestroyed(session_.bricks)) {
        session_.state = GameState::Win;
    }
}

void BreakoutGame::UpdateEndState() {
    if (IsKeyPressed(KEY_R)) {
        ResetGame();
    }
}

void BreakoutGame::UpdatePaddleMovement() {
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        session_.paddle.x -= config_.paddleSpeed;
    }

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        session_.paddle.x += config_.paddleSpeed;
    }

    ConstrainPaddleToScreen();
}

void BreakoutGame::ConstrainPaddleToScreen() {
    if (session_.paddle.x < 0.0f) {
        session_.paddle.x = 0.0f;
    }

    const float maxPaddleX = config_.screenWidth - session_.paddle.width;
    if (session_.paddle.x > maxPaddleX) {
        session_.paddle.x = maxPaddleX;
    }
}

void BreakoutGame::SyncBallWithPaddle() {
    session_.ballPosition.x = session_.paddle.x + session_.paddle.width / 2.0f;
    session_.ballPosition.y = session_.paddle.y - config_.ballRadius - config_.ballDockOffset;
}

void BreakoutGame::UpdateBallPosition() {
    session_.ballPosition.x += session_.ballSpeed.x;
    session_.ballPosition.y += session_.ballSpeed.y;
}

void BreakoutGame::HandleWallCollision() {
    if (session_.ballPosition.x - config_.ballRadius <= 0.0f) {
        session_.ballPosition.x = config_.ballRadius;
        session_.ballSpeed.x *= -1.0f;
    }

    if (session_.ballPosition.x + config_.ballRadius >= config_.screenWidth) {
        session_.ballPosition.x = config_.screenWidth - config_.ballRadius;
        session_.ballSpeed.x *= -1.0f;
    }

    if (session_.ballPosition.y - config_.ballRadius <= 0.0f) {
        session_.ballPosition.y = config_.ballRadius;
        session_.ballSpeed.y *= -1.0f;
    }
}

void BreakoutGame::HandlePaddleCollision() {
    if (!CheckCollisionCircleRec(session_.ballPosition, config_.ballRadius, session_.paddle) || session_.ballSpeed.y <= 0.0f) {
        return;
    }

    session_.ballPosition.y = session_.paddle.y - config_.ballRadius;

    const float paddleCenter = session_.paddle.x + session_.paddle.width / 2.0f;
    const float distanceFromCenter = session_.ballPosition.x - paddleCenter;

    session_.ballSpeed.y *= -1.0f;
    session_.ballSpeed.x = distanceFromCenter * 0.08f;

    if (session_.ballSpeed.x > -2.0f && session_.ballSpeed.x < 2.0f) {
        session_.ballSpeed.x = (session_.ballSpeed.x < 0.0f) ? -2.0f : 2.0f;
    }
}

void BreakoutGame::HandleBrickCollision() {
    for (Brick& brick : session_.bricks) {
        if (!brick.active || !CheckCollisionCircleRec(session_.ballPosition, config_.ballRadius, brick.rect)) {
            continue;
        }

        brick.active = false;
        session_.ballSpeed.y *= -1.0f;
        session_.score += config_.brickScore;
        break;
    }
}

void BreakoutGame::HandleLifeLoss() {
    if (session_.ballPosition.y - config_.ballRadius <= config_.screenHeight) {
        return;
    }

    --session_.lives;

    if (session_.lives > 0) {
        ResetRound();
        session_.state = GameState::Start;
        return;
    }

    session_.state = GameState::GameOver;
}

void BreakoutGame::Render() const {
    BeginDrawing();
    DrawGame(session_, config_);
    EndDrawing();
}

}  // namespace breakout
