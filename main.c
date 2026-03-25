#include <raylib.h>
#include <stdio.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <stdlib.h>
#endif
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

static bool ParseArtworkDimensionsCm(const char *path, float *widthCm, float *heightCm)
{
    int width = 0;
    int height = 0;
    int consumed = 0;

    for (const char *cursor = path; *cursor != '\0'; cursor++)
    {
        if (sscanf(cursor, "%dx%dcm%n", &width, &height, &consumed) == 2)
        {
            *widthCm = (float)width;
            *heightCm = (float)height;
            return true;
        }
    }

    return false;
}

static void ResizeArtworkImageForWeb(Image *image)
{
#if defined(PLATFORM_WEB)
    const int maxTextureEdge = 1024;
    int longestEdge = (image->width > image->height) ? image->width : image->height;

    if (longestEdge > maxTextureEdge)
    {
        float scale = (float)maxTextureEdge/(float)longestEdge;
        int resizedWidth = (int)roundf((float)image->width*scale);
        int resizedHeight = (int)roundf((float)image->height*scale);

        if (resizedWidth < 1) resizedWidth = 1;
        if (resizedHeight < 1) resizedHeight = 1;

        ImageResize(image, resizedWidth, resizedHeight);
    }
#else
    (void)image;
#endif
}

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

    const float cameraEyeHeight = 1.6f;
    float cameraYaw = PI;
    const float cameraTurnSpeed = 1.6f;   // radians per second
    const float cameraMoveSpeed = 2.0f;   // world units per second
    const float cameraLookDrop = 0.15f;

    const char *artworkPaths[] = {
        "assets/01-linnut-lampi-56x38cm.jpg",
        "assets/02-irina-rebnitskaya-syksy-76x56cm-2024.jpg",
        "assets/03-amur-and-psyke-56x38cm.jpg",
        "assets/04-joutsen-lampi-38x56cm.jpg",
        "assets/05-sergey-38x56cm.jpg",
        "assets/06-untitled-38x56cm.jpg",
        "assets/07-suihkulahde-38x56cm.jpg",
        "assets/08-suihkulahde-38x56cm.jpg",
        "assets/09-irina-rebnitskaya-afina-38x56cm-2025.jpg",
        "assets/10-gate-38x56cm.jpg",
        "assets/11-kahvila-38x56cm.jpg",
        "assets/12-aristotel-38x56cm.jpg",
        "assets/13-mama-38x56cm.jpg",
        "assets/14-proserpina-38x56cm.jpg",
        "assets/15-sun-38x56cm.jpg",
        "assets/16-maunon-puutarha-56x38cm.jpg",
        "assets/17-journeys-of-light-and-shadow-48x64cm.jpg",
    };
    enum { artworkCount = (int)(sizeof(artworkPaths)/sizeof(artworkPaths[0])) };
    Texture2D artworkTextures[artworkCount] = { 0 };
    float artworkWidthCm[artworkCount] = { 0 };
    float artworkHeightCm[artworkCount] = { 0 };
    for (int i = 0; i < artworkCount; i++)
    {
        Image artworkImage = LoadImage(artworkPaths[i]);
        ImageFlipVertical(&artworkImage);
        if (i == 8)
        {
            ImageFlipHorizontal(&artworkImage);
        }
        if (i == 4)
        {
            ImageRotateCCW(&artworkImage);
        }
        if (!ParseArtworkDimensionsCm(artworkPaths[i], &artworkWidthCm[i], &artworkHeightCm[i]))
        {
            artworkWidthCm[i] = (float)artworkImage.width;
            artworkHeightCm[i] = (float)artworkImage.height;
        }
        ResizeArtworkImageForWeb(&artworkImage);
        artworkTextures[i] = LoadTextureFromImage(artworkImage);
        UnloadImage(artworkImage);
    }
    Model wall = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model door = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    Model picture = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    SetMaterialTexture(&picture.materials[0], MATERIAL_MAP_DIFFUSE, artworkTextures[0]);
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
    const float vestibuleGalleryWallZ = roomHalfZ - vestibuleDepth;
    const float doorWidth = 1.0f;
    const float doorHeight = 2.2f;
    const float outdoorDoorCenterX = -roomHalfX + vestibuleWidth*0.5f; // on south outer wall
    const float insideDoorCenterX = outdoorDoorCenterX;                 // straight-through with outdoor door
    const float endDoorOffsetX = 0.15f;
    const float endDoorCenterX = outdoorDoorCenterX + doorWidth*0.5f + endDoorOffsetX;
    const float windowSillHeight = 0.66f;
    const float windowTopHeight = 2.6f;
    const float southPierWidth = 0.25f; // small wall between door and window area
    const float windowJambWidth = 0.2f;
    const float windowFrameWidth = 0.07f;
    const float cornerAccentSize = 0.07f;
    const Color wallColor = (Color){ 245, 243, 238, 255 };   // natural warm white
    const Color cornerColor = (Color){ 72, 76, 82, 255 };    // dark gray corner accents
    const Color smokyAluminumDoorColor = (Color){ 142, 147, 152, 255 };
    const Color northDoorColor = (Color){ 236, 236, 234, 255 };
    const Color windowFrameColor = (Color){ 250, 250, 247, 255 };
    const float pictureY = 1.6f;
    const float pictureDepth = 0.02f;
    const float pictureInset = 0.12f;
    const Color floorColor = (Color){ 150, 155, 160, 255 };  // muted medium gray
    const float centimetersToWorld = 0.016f;
    const bool artworkOnZWall[artworkCount] = {
        false, false, false,
        true, true, true, true, true, true, true,
        false, false, false, false, false, false,
        false,
    };
    const Vector3 artworkPositions[artworkCount] = {
        (Vector3){ -roomHalfX + pictureInset, pictureY, -2.35f },
        (Vector3){ -roomHalfX + pictureInset, pictureY, -1.00f },
        (Vector3){ -roomHalfX + pictureInset, pictureY, 0.35f },
        (Vector3){ -3.85f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ -1.70f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ -0.60f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ 0.50f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ 1.60f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ 2.70f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ 3.80f, pictureY, -roomHalfZ + pictureInset },
        (Vector3){ roomHalfX - pictureInset, pictureY, -2.75f },
        (Vector3){ roomHalfX - pictureInset, pictureY, -1.65f },
        (Vector3){ roomHalfX - pictureInset, pictureY, -0.55f },
        (Vector3){ roomHalfX - pictureInset, pictureY, 0.55f },
        (Vector3){ roomHalfX - pictureInset, pictureY, 1.65f },
        (Vector3){ roomHalfX - pictureInset, pictureY, 2.75f },
        (Vector3){ -roomHalfX + vestibuleWidth + pictureInset, pictureY, roomHalfZ - vestibuleDepth*0.5f },
    };
    const Vector3 artworkRotations[artworkCount] = {
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f },
        (Vector3){ 0.0f, 1.0f, 0.0f },
    };
    const float artworkAngles[artworkCount] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
    };
    bool showCorners = true;

    // Main game loop
    while (!WindowShouldClose())
    {
        float frameTime = GetFrameTime();
        Vector3 forward = { sinf(cameraYaw), 0.0f, cosf(cameraYaw) };

        if (IsKeyDown(KEY_LEFT)) cameraYaw += cameraTurnSpeed * frameTime;
        if (IsKeyDown(KEY_RIGHT)) cameraYaw -= cameraTurnSpeed * frameTime;

        forward = (Vector3){ sinf(cameraYaw), 0.0f, cosf(cameraYaw) };
        if (IsKeyDown(KEY_UP))
        {
            camera.position.x += forward.x * cameraMoveSpeed * frameTime;
            camera.position.z += forward.z * cameraMoveSpeed * frameTime;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            camera.position.x -= forward.x * cameraMoveSpeed * frameTime;
            camera.position.z -= forward.z * cameraMoveSpeed * frameTime;
        }

        camera.position.y = cameraEyeHeight;
        camera.target = (Vector3){
            camera.position.x + forward.x,
            camera.position.y - cameraLookDrop,
            camera.position.z + forward.z
        };
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

        // White window frame split into three panes
        float windowOpeningHeight = windowTopHeight - windowSillHeight;
        float windowFrameZ = roomHalfZ - wallThickness*0.25f;
        float windowFrameDepth = wallThickness*0.45f;
        float windowPaneWidth = (windowLen - 4.0f*windowFrameWidth)/3.0f;
        float windowMullion1X = windowStartX + windowPaneWidth + windowFrameWidth*1.5f;
        float windowMullion2X = windowStartX + 2.0f*windowPaneWidth + windowFrameWidth*2.5f;

        DrawModelEx(wall,
            (Vector3){ windowStartX + windowLen*0.5f, windowSillHeight + windowFrameWidth*0.5f, windowFrameZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowLen, windowFrameWidth, windowFrameDepth },
            windowFrameColor);
        DrawModelEx(wall,
            (Vector3){ windowStartX + windowLen*0.5f, windowTopHeight - windowFrameWidth*0.5f, windowFrameZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowLen, windowFrameWidth, windowFrameDepth },
            windowFrameColor);
        DrawModelEx(wall,
            (Vector3){ windowStartX + windowFrameWidth*0.5f, windowSillHeight + windowOpeningHeight*0.5f, windowFrameZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowFrameWidth, windowOpeningHeight, windowFrameDepth },
            windowFrameColor);
        DrawModelEx(wall,
            (Vector3){ roomHalfX - windowFrameWidth*0.5f, windowSillHeight + windowOpeningHeight*0.5f, windowFrameZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowFrameWidth, windowOpeningHeight, windowFrameDepth },
            windowFrameColor);
        DrawModelEx(wall,
            (Vector3){ windowMullion1X, windowSillHeight + windowOpeningHeight*0.5f, windowFrameZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowFrameWidth, windowOpeningHeight, windowFrameDepth },
            windowFrameColor);
        DrawModelEx(wall,
            (Vector3){ windowMullion2X, windowSillHeight + windowOpeningHeight*0.5f, windowFrameZ },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ windowFrameWidth, windowOpeningHeight, windowFrameDepth },
            windowFrameColor);

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
            smokyAluminumDoorColor);
        DrawModelEx(door,
                (Vector3){ outdoorDoorCenterX, doorHeight*0.5f, roomHalfZ - 0.03f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ doorWidth, doorHeight, 0.04f },
            smokyAluminumDoorColor);

        DrawModelEx(door,
                (Vector3){ endDoorCenterX, doorHeight*0.5f, -roomHalfZ + 0.03f },
            (Vector3){ 0.0f, 1.0f, 0.0f },
            0.0f,
            (Vector3){ doorWidth, doorHeight, 0.04f },
            northDoorColor);

        // Paintings around the gallery perimeter
        for (int i = 0; i < artworkCount; i++)
        {
            float pictureWidth = artworkWidthCm[i]*centimetersToWorld;
            float pictureHeight = artworkHeightCm[i]*centimetersToWorld;
            Vector3 pictureScale = artworkOnZWall[i]
                ? (Vector3){ pictureWidth, pictureHeight, pictureDepth }
                : (Vector3){ pictureDepth, pictureHeight, pictureWidth };

            SetMaterialTexture(&picture.materials[0], MATERIAL_MAP_DIFFUSE, artworkTextures[i]);
            DrawModelEx(picture,
                artworkPositions[i],
                artworkRotations[i],
                artworkAngles[i],
                pictureScale,
                WHITE);
        }

        DrawGrid(20, 1.0f);
        EndMode3D();

        EndDrawing();
    }

    UnloadModel(wall);
    UnloadModel(door);
    UnloadModel(picture);
    for (int i = 0; i < artworkCount; i++) UnloadTexture(artworkTextures[i]);

    // De-Initialization
    CloseWindow(); // Close the window and OpenGL context

    return 0;
}
