#include <raylib.h>
#include <stdio.h>
#define WIDTH 800
#define HEIGHT 600

typedef enum type{
    ImageType,
} type_t;

typedef struct {
    type_t type;
} graphics;

typedef struct{
    graphics header;
    unsigned char *fileName; 
} image_t;

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "Minimal Raylib Example");

    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 4.0f, 4.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Image image = LoadImage("IMG_1.jpeg");
    Texture2D texture = LoadTextureFromImage(image);
    Model cube = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
    SetMaterialTexture(&cube.materials[0], MATERIAL_MAP_DIFFUSE, texture);
    Model wall = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model frame = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model picture = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    SetMaterialTexture(&picture.materials[0], MATERIAL_MAP_DIFFUSE, texture);
    Vector3 cubePosition = (Vector3){ 0.0f, 1.0f, 0.0f };
    const float roomHalfSize = 6.0f;
    const float wallHeight = 3.0f;
    const float wallThickness = 0.2f;
    const Color wallColor = (Color){ 245, 243, 238, 255 };   // natural warm white
    const Color frameColor = (Color){ 110, 85, 60, 255 };    // wood-like brown
    const float pictureY = 1.6f;
    const float frameDepth = 0.08f;
    const float pictureDepth = 0.03f;
    const float frameInset = 0.12f;    // distance from wall center toward room interior
    const float pictureInset = 0.18f;  // slightly more interior than frame so image stays visible

    // Main game loop
    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        // Back wall (z-)
        DrawModelEx(wall,
                (Vector3){ 0.0f, wallHeight*0.5f, -roomHalfSize },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ roomHalfSize*2.0f, wallHeight, wallThickness },
            wallColor);
        // Front wall (z+)
        DrawModelEx(wall,
                (Vector3){ 0.0f, wallHeight*0.5f, roomHalfSize },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ roomHalfSize*2.0f, wallHeight, wallThickness },
            wallColor);
        // Left wall (x-)
        DrawModelEx(wall,
                (Vector3){ -roomHalfSize, wallHeight*0.5f, 0.0f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ wallThickness, wallHeight, roomHalfSize*2.0f },
            wallColor);
        // Right wall (x+)
        DrawModelEx(wall,
                (Vector3){ roomHalfSize, wallHeight*0.5f, 0.0f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ wallThickness, wallHeight, roomHalfSize*2.0f },
            wallColor);

        // Framed picture on each wall
        // Back wall (z-)
        DrawModelEx(frame,
            (Vector3){ 0.0f, pictureY, -roomHalfSize + frameInset },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ 2.4f, 1.8f, frameDepth },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ 0.0f, pictureY, -roomHalfSize + pictureInset },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ 2.0f, 1.4f, pictureDepth },
            WHITE);

        // Front wall (z+)
        DrawModelEx(frame,
            (Vector3){ 0.0f, pictureY, roomHalfSize - frameInset },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ 2.4f, 1.8f, frameDepth },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ 0.0f, pictureY, roomHalfSize - pictureInset },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ 2.0f, 1.4f, pictureDepth },
            WHITE);

        // Left wall (x-)
        DrawModelEx(frame,
            (Vector3){ -roomHalfSize + frameInset, pictureY, 0.0f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ frameDepth, 1.8f, 2.4f },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ -roomHalfSize + pictureInset, pictureY, 0.0f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ pictureDepth, 1.4f, 2.0f },
            WHITE);

        // Right wall (x+)
        DrawModelEx(frame,
            (Vector3){ roomHalfSize - frameInset, pictureY, 0.0f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ frameDepth, 1.8f, 2.4f },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ roomHalfSize - pictureInset, pictureY, 0.0f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ pictureDepth, 1.4f, 2.0f },
            WHITE);

        DrawModel(cube, cubePosition, 1.0f, WHITE);
        DrawModelWires(cube, cubePosition, 1.0f, RAYWHITE);
        DrawGrid(20, 1.0f);
        EndMode3D();

        char buffer[256];
        long time = GetTime();
        snprintf(buffer, sizeof(buffer), "Hello, Raylib! %ld seconds", time);
        DrawText(buffer, 20, 20, 20, LIGHTGRAY);
        DrawText("Mouse wheel: zoom | Right mouse: orbit", 20, 50, 18, GRAY);

        EndDrawing();
    }

    UnloadModel(cube);
    UnloadModel(wall);
    UnloadModel(frame);
    UnloadModel(picture);
    UnloadImage(image);
    UnloadTexture(texture);

    // De-Initialization
    CloseWindow(); // Close the window and OpenGL context

    return 0;
}
