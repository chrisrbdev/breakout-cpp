#pragma once

#include <vector>

#include "domain/Brick.h"
#include "domain/GameState.h"
#include "raylib.h"

namespace breakout {

struct GameSession {
    int score = 0;
    int lives = 0;
    Rectangle paddle{};
    Vector2 ballPosition{};
    Vector2 ballSpeed{};
    std::vector<Brick> bricks;
    GameState state = GameState::Start;
};

}  // namespace breakout
