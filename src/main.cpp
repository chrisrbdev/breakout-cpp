#include "raylib.h"

int main() {
    constexpr int screenWidth = 800;
    constexpr int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "breakout-cpp-raylib");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Your project is ready!", 220, 200, 30, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
