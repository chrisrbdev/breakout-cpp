#pragma once

#include "audio/ProceduralMusicEngine.h"
#include "core/GameConfig.h"
#include "core/GameSession.h"

namespace breakout {

class BreakoutGame {
public:
    BreakoutGame();
    ~BreakoutGame();

    void Run();

private:
    void ResetGame();
    void ResetRound();
    void Update();
    void UpdateStartState();
    void UpdatePlayingState();
    void UpdateEndState();
    void UpdatePaddleMovement();
    void ConstrainPaddleToScreen();
    void SyncBallWithPaddle();
    void UpdateBallPosition();
    void HandleWallCollision();
    void HandlePaddleCollision();
    void HandleBrickCollision();
    void HandleLifeLoss();
    void Render() const;

    GameConfig config_{};
    GameSession session_{};
    ProceduralMusicEngine musicEngine_{};
    bool hasProceduralMusic_ = false;
};

}  // namespace breakout
