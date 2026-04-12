#include "core/BrickFactory.h"

namespace breakout {

std::vector<Brick> CreateBricks(const BrickLayout& layout) {
    std::vector<Brick> bricks;
    bricks.reserve(layout.rows * layout.columns);

    for (int row = 0; row < layout.rows; ++row) {
        for (int column = 0; column < layout.columns; ++column) {
            Brick brick;
            brick.rect = {
                layout.startX + column * (layout.width + layout.padding),
                layout.startY + row * (layout.height + layout.padding),
                layout.width,
                layout.height
            };
            bricks.push_back(brick);
        }
    }

    return bricks;
}

bool AreAllBricksDestroyed(const std::vector<Brick>& bricks) {
    for (const Brick& brick : bricks) {
        if (brick.active) {
            return false;
        }
    }

    return true;
}

}  // namespace breakout
