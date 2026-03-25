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

std::vector<Brick> CreateBricks() {
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
                brickHeight
            };
            brick.active = true;
            bricks.push_back(brick);
        }
    }
    return bricks;
}

void ResetBall(Vector2& ballPosition, Vector2& ballSpeed, const Rectangle& paddle, float ballRadius) {
    ballPosition.x = paddle.x + paddle.width / 2.0f;
    ballPosition.y = paddle.y - ballRadius - 2.0f;
    ballSpeed = { 5.0f, -5.0f };
}

void ResetPaddle(Rectangle &paddle, int screenWidth, int screenHeight) {
    paddle.width = 140.0f;
    paddle.height = 20.0f;
    paddle.x = (screenWidth - paddle.width) / 2.0f;
    paddle.y = screenHeight - 40.0f;
}

bool AreAllBricksDestroyed(const std::vector<Brick> &bricks) {
    for (const Brick &brick : bricks) {
        if (brick.active)
            return false;
    }
    return true;
}

Color GetBrickColorByRow(int row) {
    switch (row) {
    case 0:
        return RED;
    case 1:
        return ORANGE;
    case 2:
        return YELLOW;
    case 3:
        return GREEN;
    case 4:
        return BLUE;
    default:
        return SKYBLUE;
    }
}

int main() {
    int score = 0;
    int lives = 3;
    const int screenWidth = 960;
    const int screenHeight = 540;
    const float paddleSpeed = 8.0f;
    const float ballRadius = 10.0f;

    InitWindow(screenWidth, screenHeight, "cpp-breakout-raylib");
    SetTargetFPS(60);

    Rectangle paddle = {0};
    ResetPaddle(paddle, screenWidth, screenHeight);

    Vector2 ballPosition = { 0 };
    Vector2 ballSpeed = { 0 };
    ResetBall(ballPosition, ballSpeed, paddle, ballRadius);

    std::vector<Brick> bricks = CreateBricks();

    GameState gameState = START;
    while (!WindowShouldClose()) {
        // Update
        if (gameState == START) {
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
                paddle.x -= paddleSpeed;

            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
                paddle.x += paddleSpeed;

            if (paddle.x < 0)
                paddle.x = 0;
            if (paddle.x + paddle.width > screenWidth)
                paddle.x = screenWidth - paddle.width;

            ballPosition.x = paddle.x + paddle.width / 2.0f;
            ballPosition.y = paddle.y - ballRadius - 2.0f;

            if (IsKeyPressed(KEY_SPACE)) {
                ballSpeed = { 5.0f, -5.0f };
                gameState = PLAYING;
            }
        }
        else if (gameState == PLAYING) {
            // Paddle movement
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
                paddle.x -= paddleSpeed;

            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
                paddle.x += paddleSpeed;

            // Paddle bounds
            if (paddle.x < 0)
                paddle.x = 0;

            if (paddle.x + paddle.width > screenWidth)
                paddle.x = screenWidth - paddle.width;
            // Ball movement
            ballPosition.x += ballSpeed.x;
            ballPosition.y += ballSpeed.y;

            // Wall collision
            if (ballPosition.x - ballRadius <= 0) {
                ballPosition.x = ballRadius;
                ballSpeed.x *= -1.0f;
            }

            if (ballPosition.x + ballRadius >= screenWidth) {
                ballPosition.x = screenWidth - ballRadius;
                ballSpeed.x *= -1.0f;
            }

            if (ballPosition.y - ballRadius <= 0) {
                ballPosition.y = ballRadius;
                ballSpeed.y *= -1.0f;
            }

            // PADDLE COLLISION
            if (CheckCollisionCircleRec(ballPosition, ballRadius, paddle) && ballSpeed.y > 0) {
                // CheckCollisionCircleRec is native function from raylib.h
                ballPosition.y = paddle.y - ballRadius;
                float paddleCenter = paddle.x + paddle.width / 2.0f;
                float distanceFromCenter = ballPosition.x - paddleCenter;
                ballSpeed.y *= -1.0f;
                ballSpeed.x = distanceFromCenter * 0.08f;

                if (ballSpeed.x > -2.0f && ballSpeed.x < 2.0f) {
                    ballSpeed.x = (ballSpeed.x < 0) ? -2.0f : 2.0f;
                }
            }

            // BRICK COLLISION
            for (Brick &brick : bricks) {
                if (brick.active && CheckCollisionCircleRec(ballPosition, ballRadius, brick.rect)) {
                    brick.active = false;
                    ballSpeed.y *= -1.0f;
                    score += 100;
                    break;
                }
            }

            // Ball fell below the screen
            if (ballPosition.y - ballRadius > screenHeight) {
                lives--;
                if (lives > 0) {
                    ResetPaddle(paddle, screenWidth, screenHeight);
                    ResetBall(ballPosition, ballSpeed, paddle, ballRadius);
                    gameState = START;
                }
                else
                    gameState = GAME_OVER;
            }

            // Win condition
            if (AreAllBricksDestroyed(bricks)) {
                gameState = WIN;
            }
        }
        else if (gameState == WIN || gameState == GAME_OVER) {
            if (IsKeyPressed(KEY_R))
            {
                score = 0;
                lives = 3;
                bricks = CreateBricks();
                ResetPaddle(paddle, screenWidth, screenHeight);
                ResetBall(ballPosition, ballSpeed, paddle, ballRadius);
                gameState = START;
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw bricks
        for (int i = 0; i < (int)bricks.size(); i++) {
            if (bricks[i].active) {
                int row = i / 10;
                DrawRectangleRec(bricks[i].rect, GetBrickColorByRow(row));
            }
        }

        // Draw paddle and ball
        DrawRectangleRec(paddle, WHITE);
        DrawCircleV(ballPosition, ballRadius, YELLOW);
        // UI
        DrawText(TextFormat("Score: %i", score), 20, 20, 20, RAYWHITE);
        DrawText(TextFormat("Lives: %i", lives), 840, 20, 20, RAYWHITE);

        // State messages
        if (gameState == START) {
            DrawText("Press SPACE to start", 310, 250, 30, GREEN);
        }
        else if (gameState == WIN) {
            DrawText("YOU WIN!", 360, 220, 40, YELLOW);
            DrawText("Press R to restart", 335, 280, 24, RAYWHITE);
        }
        else if (gameState == GAME_OVER) {
            DrawText("GAME OVER", 335, 220, 40, RED);
            DrawText("Press R to restart", 335, 280, 24, RAYWHITE);
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
