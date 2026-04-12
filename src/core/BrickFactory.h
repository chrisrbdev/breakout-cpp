#pragma once

#include <vector>

#include "domain/Brick.h"
#include "core/GameConfig.h"

namespace breakout {

std::vector<Brick> CreateBricks(const BrickLayout& layout);
bool AreAllBricksDestroyed(const std::vector<Brick>& bricks);

}  // namespace breakout
