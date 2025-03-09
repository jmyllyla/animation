#include "raylib.h"

int main(void)
{
    // Initialize the window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Minimal Raylib Example");

    // Set the background color
    SetTargetFPS(60);  // Set the FPS to 60

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        // (Nothing to update for this minimal example)

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);  // Clear the screen with a white color

        DrawText("Hello, Raylib!", 200, 200, 20, DARKGRAY);  // Draw some text

        EndDrawing();
    }

    // De-Initialization
    CloseWindow(); // Close the window and OpenGL context

    return 0;
}
