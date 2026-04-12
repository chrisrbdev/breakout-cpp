#pragma once

#include "raylib.h"

namespace breakout {

struct Brick {
    Rectangle rect{};
    bool active = true;
};

}  // namespace breakout
