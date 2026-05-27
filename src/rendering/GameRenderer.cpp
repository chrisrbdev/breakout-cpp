#include "rendering/GameRenderer.h"

namespace breakout {
namespace {

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

void DrawCenteredText(const char* text, int y, int fontSize, Color color, int screenWidth) {
    const int textWidth = MeasureText(text, fontSize);
    DrawText(text, (screenWidth - textWidth) / 2, y, fontSize, color);
}

void DrawHud(const GameSession& session, const GameConfig& config) {
    const int margin = 20;
    const int scoreFontSize = 22;
    const int statusFontSize = 14;
    const int livesFontSize = 20;

    const char* scoreText = TextFormat("Score: %i", session.score);
    const int scoreTextWidth = MeasureText(scoreText, scoreFontSize);
    DrawText(scoreText, (config.screenWidth - scoreTextWidth) / 2, 16, scoreFontSize, RAYWHITE);

    const char* livesText = TextFormat("Lives: %i", session.lives);
    const int livesTextWidth = MeasureText(livesText, livesFontSize);
    DrawText(livesText, config.screenWidth - livesTextWidth - margin, 20, livesFontSize, RAYWHITE);

    if (!session.musicEnabled) {
        DrawText("Procedural music unavailable", margin, 20, statusFontSize, ORANGE);
        return;
    }

    DrawText("M: mute/unmute music", margin, 20, statusFontSize, DARKGRAY);
    DrawText(session.musicMuted ? "Music: OFF" : "Music: ON", margin, 38, statusFontSize,
             session.musicMuted ? ORANGE : GREEN);
}

void DrawStateMessage(GameState state, int screenWidth) {
    switch (state) {
        case GameState::Start:
            DrawCenteredText("Press SPACE to start", 250, 30, GREEN, screenWidth);
            break;
        case GameState::Win:
            DrawCenteredText("YOU WIN!", 220, 40, YELLOW, screenWidth);
            DrawCenteredText("Press R to restart", 280, 24, RAYWHITE, screenWidth);
            break;
        case GameState::GameOver:
            DrawCenteredText("GAME OVER", 220, 40, RED, screenWidth);
            DrawCenteredText("Press R to restart", 280, 24, RAYWHITE, screenWidth);
            break;
        case GameState::Playing:
            break;
    }
}

}  // namespace

void DrawGame(const GameSession& session, const GameConfig& config) {
    ClearBackground(BLACK);
    DrawHud(session, config);

    for (int index = 0; index < static_cast<int>(session.bricks.size()); ++index) {
        if (!session.bricks[index].active) {
            continue;
        }

        const int row = index / config.brickLayout.columns;
        DrawRectangleRec(session.bricks[index].rect, GetBrickColorByRow(row));
    }

    DrawRectangleRec(session.paddle, WHITE);
    DrawCircleV(session.ballPosition, config.ballRadius, YELLOW);
    DrawStateMessage(session.state, config.screenWidth);
}

}  // namespace breakout
