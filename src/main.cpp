#include "raylib.h"
#include <vector>

enum GameState {
    START,
    PLAYING,
    WIN,
    GAME_OVER
};

struct Brick {
    Rectangle rect;
    bool active;
};

void ResetBall(Vector2& ballPosition, Vector2& ballSpeed) {
    ballPosition = { 470, 300 };
    ballSpeed = { 5.0f, -5.0f };
}

void ResetPaddle(Rectangle& paddle) {
    paddle.x = 400;
    paddle.y = 500;
}

int main() {
    GameState gameState = START;
    int score = 0;
    int lives = 3;
    const int screenWidth = 960;
    const int screenHeight = 540;

    InitWindow(screenWidth, screenHeight, "cpp-breakout-raylib");
    SetTargetFPS(60);

    Rectangle paddle = {400, 500, 140, 20};

    Vector2 ballPosition = {470, 300};
    Vector2 ballSpeed = {4.0f, -4.0f};
    float ballRadius = 10.0f;

    std::vector<Brick> bricks;

    const int rows = 5;
    const int cols = 10;
    const float brickWidth = 80.0f;
    const float brickHeight = 25.0f;
    const float brickPadding = 10.0f;
    const float startX = 35.0f;
    const float startY = 60.0f;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Brick brick;
            brick.rect = {
                startX + col * (brickWidth + brickPadding),
                startY + row * (brickHeight + brickPadding),
                brickWidth,
                brickHeight};
            brick.active = true;
            bricks.push_back(brick);
        }
    }

    while (!WindowShouldClose()) {
        // Update
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            paddle.x -= 8.0f;
        }

        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            paddle.x += 8.0f;
        }

        if (paddle.x < 0) {
            paddle.x = 0;
        }

        if (paddle.x + paddle.width > screenWidth) {
            paddle.x = screenWidth - paddle.width;
        }

        ballPosition.x += ballSpeed.x;
        ballPosition.y += ballSpeed.y;

        if (ballPosition.x - ballRadius <= 0 || ballPosition.x + ballRadius >= screenWidth) {
            ballSpeed.x *= -1.0f;
        }

        if (ballPosition.y - ballRadius <= 0) {
            ballSpeed.y *= -1.0f;
        }

        if (ballPosition.y - ballRadius > screenHeight) {
            ResetPaddle(paddle);
            ResetBall(ballPosition, ballSpeed);
        }

        // PADDLE COLLISION
        if (CheckCollisionCircleRec(ballPosition, ballRadius, paddle) && ballSpeed.y > 0) {
            //CheckCollisionCircleRec is native function from raylib.h
            float paddleCenter = paddle.x + paddle.width / 2.0f;
            float distanceFromCenter = ballPosition.x - paddleCenter;

            ballSpeed.y *= -1.0f;
            ballSpeed.x = distanceFromCenter * 0.08f;
        }

        // BRICK COLLISION
        for (Brick& brick : bricks) {
            if (brick.active && CheckCollisionCircleRec(ballPosition, ballRadius, brick.rect)) {
                brick.active = false;
                ballSpeed.y *= -1.0f;
                break;
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleRec(paddle, WHITE);
        DrawCircleV(ballPosition, ballRadius, YELLOW);

        for (const Brick &brick : bricks) {
            if (brick.active) {
                DrawRectangleRec(brick.rect, BLUE);
            }
        }

        DrawText("Breakout!", 20, 20, 20, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
