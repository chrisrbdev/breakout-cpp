#pragma once

namespace breakout {

struct BrickLayout {
    int rows = 5;
    int columns = 10;
    float width = 80.0f;
    float height = 25.0f;
    float padding = 10.0f;
    float startX = 35.0f;
    float startY = 60.0f;
};

struct GameConfig {
    int screenWidth = 960;
    int screenHeight = 540;
    int targetFps = 60;
    int initialLives = 3;
    int brickScore = 100;
    float paddleWidth = 140.0f;
    float paddleHeight = 20.0f;
    float paddleSpeed = 8.0f;
    float paddleBottomOffset = 40.0f;
    float ballRadius = 10.0f;
    float ballLaunchSpeed = -5.0f;
    float ballDockOffset = 2.0f;
    BrickLayout brickLayout{};
};

}  // namespace breakout
