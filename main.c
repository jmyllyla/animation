#include <raylib.h>
#include <stdio.h>
#include <math.h>
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
    camera.position = (Vector3){ 0.0f, 1.6f, 3.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Custom orbit parameters
    float orbitAngle = 0.0f;
    const float orbitRadius = 3.0f;
    const float orbitHeight = 1.6f;
    const float orbitSpeed = 0.1f;  // radians per second

    Image image = LoadImage("IMG_1.jpeg");
    ImageFlipVertical(&image);
    Texture2D texture = LoadTextureFromImage(image);
    Model wall = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model door = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model frame = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model picture = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    SetMaterialTexture(&picture.materials[0], MATERIAL_MAP_DIFFUSE, texture);
    const float sizeScale = 1.5f;     // make walls 50% longer
    const float roomWidth = 6.2f*sizeScale;     // north wall length on X-axis
    const float roomDepth = 4.7f*sizeScale;     // side wall length on Z-axis
    const float roomHalfX = roomWidth*0.5f;
    const float roomHalfZ = roomDepth*0.5f;
    const float wallHeight = 3.0f;
    const float wallThickness = 0.2f;

    // Small corner room (south-west vestibule)
    const float vestibuleWidth = 1.6f*sizeScale;    // along X into room
    const float vestibuleDepth = 1.4f*sizeScale;    // along Z into room
    const float doorWidth = 1.0f;
    const float doorHeight = 2.2f;
    const float outdoorDoorCenterX = -roomHalfX + vestibuleWidth*0.5f; // on south outer wall
    const float insideDoorCenterX = outdoorDoorCenterX;                 // straight-through with outdoor door
    const float endDoorCenterX = outdoorDoorCenterX + doorWidth*0.5f;   // shifted right by half door width
    const float windowSillHeight = 1.08f;
    const float windowTopHeight = 2.6f;
    const float southPierWidth = 0.25f; // small wall between door and window area
    const float windowJambWidth = 0.2f;
    const float cornerAccentSize = 0.07f;
    const Color wallColor = (Color){ 245, 243, 238, 255 };   // natural warm white
    const Color cornerColor = (Color){ 72, 76, 82, 255 };    // dark gray corner accents
    const Color doorColor = (Color){ 160, 120, 80, 255 };
    const Color beigeDoorColor = (Color){ 210, 190, 155, 255 };
    const Color frameColor = (Color){ 110, 85, 60, 255 };    // wood-like brown
    const float pictureY = 1.6f;
    const float frameDepth = 0.08f;
    const float pictureDepth = 0.03f;
    const float frameInset = 0.12f;    // distance from wall center toward room interior
    const float pictureInset = 0.18f;  // slightly more interior than frame so image stays visible
    const Color floorColor = (Color){ 150, 155, 160, 255 };  // muted medium gray
    bool showCorners = true;

    // Main game loop
    while (!WindowShouldClose())
    {
        // Custom slow orbit around room center
        orbitAngle += orbitSpeed * GetFrameTime();
        camera.position.x = sinf(orbitAngle) * orbitRadius;
        camera.position.z = cosf(orbitAngle) * orbitRadius;
        camera.position.y = orbitHeight;
        if (IsKeyPressed(KEY_C)) showCorners = !showCorners;

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ roomWidth, roomDepth }, floorColor);

        // North wall split for end-wall door opening
        float northSegWestLen = endDoorCenterX - doorWidth*0.5f + roomHalfX;
        float northSegEastLen = roomHalfX - (endDoorCenterX + doorWidth*0.5f);

        DrawModelEx(wall,
                (Vector3){ -roomHalfX + northSegWestLen*0.5f, wallHeight*0.5f, -roomHalfZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ northSegWestLen, wallHeight, wallThickness },
            wallColor);
        DrawModelEx(wall,
                (Vector3){ endDoorCenterX + doorWidth*0.5f + northSegEastLen*0.5f, wallHeight*0.5f, -roomHalfZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ northSegEastLen, wallHeight, wallThickness },
            wallColor);
        DrawModelEx(wall,
                (Vector3){ endDoorCenterX, doorHeight + (wallHeight - doorHeight)*0.5f, -roomHalfZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ doorWidth, wallHeight - doorHeight, wallThickness },
            wallColor);

        // East wall (full)
        DrawModelEx(wall,
                (Vector3){ roomHalfX, wallHeight*0.5f, 0.0f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ wallThickness, wallHeight, roomHalfZ*2.0f },
            wallColor);

        // West wall (full)
        DrawModelEx(wall,
                (Vector3){ -roomHalfX, wallHeight*0.5f, 0.0f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
                (Vector3){ wallThickness, wallHeight, roomHalfZ*2.0f },
            wallColor);

        // South wall: outdoor door + mostly window (small top/bottom wall strips)
        float southSegWestLen = outdoorDoorCenterX - doorWidth*0.5f + roomHalfX;
        float southDoorRightX = outdoorDoorCenterX + doorWidth*0.5f;
        float windowStartX = southDoorRightX + southPierWidth;
        float windowLen = roomHalfX - windowStartX;

        DrawModelEx(wall,
                (Vector3){ -roomHalfX + southSegWestLen*0.5f, wallHeight*0.5f, roomHalfZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
            (Vector3){ southSegWestLen, wallHeight, wallThickness },
            wallColor);

        // Small pier between the outdoor door and the large window opening
        DrawModelEx(wall,
            (Vector3){ southDoorRightX + southPierWidth*0.5f, wallHeight*0.5f, roomHalfZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ southPierWidth, wallHeight, wallThickness },
            wallColor);

        DrawModelEx(wall,
                (Vector3){ outdoorDoorCenterX, doorHeight + (wallHeight - doorHeight)*0.5f, roomHalfZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ doorWidth, wallHeight - doorHeight, wallThickness },
            wallColor);

        // Window wall components: bottom strip, top strip, right jamb
        DrawModelEx(wall,
            (Vector3){ windowStartX + windowLen*0.5f, windowSillHeight*0.5f, roomHalfZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowLen, windowSillHeight, wallThickness },
            wallColor);
        DrawModelEx(wall,
            (Vector3){ windowStartX + windowLen*0.5f, windowTopHeight + (wallHeight - windowTopHeight)*0.5f, roomHalfZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowLen, wallHeight - windowTopHeight, wallThickness },
            wallColor);
        DrawModelEx(wall,
            (Vector3){ roomHalfX - windowJambWidth*0.5f, windowSillHeight + (windowTopHeight - windowSillHeight)*0.5f, roomHalfZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowJambWidth, windowTopHeight - windowSillHeight, wallThickness },
            wallColor);

        // Vestibule interior partition walls (forming the small corner room)
        float vestibuleX = -roomHalfX + vestibuleWidth;
        float vestibuleZ = roomHalfZ - vestibuleDepth;

        // North partition wall of vestibule split for inside door opening (straight-through with south door)
        float partWestLen = insideDoorCenterX - doorWidth*0.5f + roomHalfX;
        float partEastLen = vestibuleX - (insideDoorCenterX + doorWidth*0.5f);

        DrawModelEx(wall,
                (Vector3){ -roomHalfX + partWestLen*0.5f, wallHeight*0.5f, vestibuleZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ partWestLen, wallHeight, wallThickness },
            wallColor);
        DrawModelEx(wall,
                (Vector3){ insideDoorCenterX + doorWidth*0.5f + partEastLen*0.5f, wallHeight*0.5f, vestibuleZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ partEastLen, wallHeight, wallThickness },
            wallColor);
        DrawModelEx(wall,
                (Vector3){ insideDoorCenterX, doorHeight + (wallHeight - doorHeight)*0.5f, vestibuleZ },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ doorWidth, wallHeight - doorHeight, wallThickness },
            wallColor);

        // East partition wall of vestibule (full)
        DrawModelEx(wall,
                (Vector3){ vestibuleX, wallHeight*0.5f, roomHalfZ - vestibuleDepth*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ wallThickness, wallHeight, vestibuleDepth },
            wallColor);

        // Dark gray corner accents (toggle with C)
        if (showCorners)
        {
            DrawModelEx(wall,
                (Vector3){ -roomHalfX + wallThickness*0.5f, wallHeight*0.5f, -roomHalfZ + wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ roomHalfX - wallThickness*0.5f, wallHeight*0.5f, -roomHalfZ + wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ roomHalfX - wallThickness*0.5f, wallHeight*0.5f, roomHalfZ - wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ -roomHalfX + wallThickness*0.5f, wallHeight*0.5f, roomHalfZ - wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ -roomHalfX + wallThickness*0.5f, wallHeight*0.5f, vestibuleZ + wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ vestibuleX - wallThickness*0.5f, wallHeight*0.5f, vestibuleZ + wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ vestibuleX - wallThickness*0.5f, wallHeight*0.5f, roomHalfZ - wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ vestibuleX + wallThickness*0.5f, wallHeight*0.5f, roomHalfZ - wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            // Gallery-facing accents for the small-room notch corners
            DrawModelEx(wall,
                (Vector3){ -roomHalfX + wallThickness*0.5f, wallHeight*0.5f, vestibuleZ - wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
            DrawModelEx(wall,
                (Vector3){ vestibuleX - wallThickness*0.5f, wallHeight*0.5f, vestibuleZ - wallThickness*0.5f },
                (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ cornerAccentSize, wallHeight, cornerAccentSize },
                cornerColor);
        }

        // Door leaves (for visual cue)
        DrawModelEx(door,
                (Vector3){ insideDoorCenterX, doorHeight*0.5f, vestibuleZ + 0.03f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
                0.0f,
                (Vector3){ doorWidth, doorHeight, 0.04f },
            doorColor);
        DrawModelEx(door,
                (Vector3){ outdoorDoorCenterX, doorHeight*0.5f, roomHalfZ - 0.03f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ doorWidth, doorHeight, 0.04f },
            doorColor);

        DrawModelEx(door,
                (Vector3){ endDoorCenterX, doorHeight*0.5f, -roomHalfZ + 0.03f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ doorWidth, doorHeight, 0.04f },
            beigeDoorColor);

        // Framed pictures on each wall (scaled down)
        // Back wall (z-) — 1 picture, centered
        DrawModelEx(frame,
            (Vector3){ 0.0f, pictureY, -roomHalfZ + frameInset },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ 1.4f, 1.1f, frameDepth },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ 0.0f, pictureY, -roomHalfZ + pictureInset },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ 1.1f, 0.8f, pictureDepth },
            WHITE);

        // Left wall (x-) — 2 pictures, shifted north to avoid vestibule
        DrawModelEx(frame,
            (Vector3){ -roomHalfX + frameInset, pictureY, -1.8f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ frameDepth, 1.1f, 1.4f },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ -roomHalfX + pictureInset, pictureY, -1.8f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ pictureDepth, 0.8f, 1.1f },
            WHITE);
        DrawModelEx(frame,
            (Vector3){ -roomHalfX + frameInset, pictureY, 0.2f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ frameDepth, 1.1f, 1.4f },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ -roomHalfX + pictureInset, pictureY, 0.2f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ pictureDepth, 0.8f, 1.1f },
            WHITE);

        // Right wall (x+) — 2 pictures, evenly spaced
        DrawModelEx(frame,
            (Vector3){ roomHalfX - frameInset, pictureY, -1.3f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ frameDepth, 1.1f, 1.4f },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ roomHalfX - pictureInset, pictureY, -1.3f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ pictureDepth, 0.8f, 1.1f },
            WHITE);
        DrawModelEx(frame,
            (Vector3){ roomHalfX - frameInset, pictureY, 1.3f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ frameDepth, 1.1f, 1.4f },
            frameColor);
        DrawModelEx(picture,
            (Vector3){ roomHalfX - pictureInset, pictureY, 1.3f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ pictureDepth, 0.8f, 1.1f },
            WHITE);

        DrawGrid(20, 1.0f);
        EndMode3D();

        char buffer[256];
        long time = GetTime();
        snprintf(buffer, sizeof(buffer), "%ld seconds", time);
        DrawText(buffer, 20, 20, 20, LIGHTGRAY);
        DrawText("Room scaled x1.5, straight-through vestibule doors, south wall mostly window", 20, 50, 18, GRAY);
        DrawText("Mouse wheel: zoom | Right mouse: orbit", 20, 74, 18, GRAY);
        DrawText("Press C: toggle corner accents", 20, 98, 18, GRAY);

        EndDrawing();
    }

    UnloadModel(wall);
    UnloadModel(door);
    UnloadModel(frame);
    UnloadModel(picture);
    UnloadImage(image);
    UnloadTexture(texture);

    // De-Initialization
    CloseWindow(); // Close the window and OpenGL context

    return 0;
}
