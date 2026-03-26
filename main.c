#include <raylib.h>
#include <stdio.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
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

typedef enum {
    VirtualControlMoveForward = 0,
    VirtualControlMoveBackward,
    VirtualControlTurnLeft,
    VirtualControlTurnRight,
    VirtualControlStepLeft,
    VirtualControlStepRight,
    VirtualControlCount,
} virtual_control_t;

#if defined(PLATFORM_WEB)
static bool virtualControlStates[VirtualControlCount] = { false };

EM_JS(void, ReportWebLoaderStage, (const char *stage), {
    if (typeof window === 'undefined') return;
    if (typeof window.__galleryLoaderStage !== 'function') return;
    window.__galleryLoaderStage(UTF8ToString(stage));
});
#endif

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

static float Clamp01(float value);

static void ResizeArtworkImageForWeb(Image *image)
{
#if defined(PLATFORM_WEB)
    const int maxTextureEdge = 2048;
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

static unsigned int Hash2D(unsigned int x, unsigned int y)
{
    unsigned int hash = x*374761393u + y*668265263u;
    hash = (hash ^ (hash >> 13))*1274126177u;
    return hash ^ (hash >> 16);
}

static float Hash01(unsigned int x, unsigned int y)
{
    return (float)(Hash2D(x, y) & 0x00ffffffu)/(float)0x01000000u;
}

static unsigned char ClampColorComponent(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

static Texture2D CreateWallTexture(void)
{
    const int textureSize = 192;
    Color *pixels = (Color *)MemAlloc(textureSize*textureSize*sizeof(Color));

    for (int y = 0; y < textureSize; y++)
    {
        for (int x = 0; x < textureSize; x++)
        {
            float u = ((float)x + 0.5f)/(float)textureSize;
            float v = ((float)y + 0.5f)/(float)textureSize;
            float edgeDistance = fminf(fminf(u, 1.0f - u), fminf(v, 1.0f - v));
            float panelFrame = Clamp01((0.06f - edgeDistance)/0.06f);
            float centerLift = sinf(u*PI)*sinf(v*PI);
            float coarseNoise = Hash01((unsigned int)(x/10), (unsigned int)(y/10))*2.0f - 1.0f;
            float fineNoise = Hash01((unsigned int)x, (unsigned int)y)*2.0f - 1.0f;
            float brushedVariation = sinf((float)x*0.18f + Hash01(0u, (unsigned int)(y/12))*PI)*0.6f
                + sinf((float)y*0.09f)*0.35f;
            float brightness = 244.7f
                + centerLift*0.7f
                - panelFrame*1.2f
                + coarseNoise*0.55f
                + fineNoise*0.18f
                + brushedVariation*0.22f;
            int shade = (int)roundf(brightness);
            pixels[y*textureSize + x] = (Color){
                ClampColorComponent(shade),
                ClampColorComponent(shade),
                ClampColorComponent(shade - 1),
                255
            };
        }
    }

    Image textureImage = {
        .data = pixels,
        .width = textureSize,
        .height = textureSize,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    Texture2D texture = LoadTextureFromImage(textureImage);
    UnloadImage(textureImage);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);

    return texture;
}

#if defined(PLATFORM_WEB)
EMSCRIPTEN_KEEPALIVE
void SetVirtualControlState(int control, int isPressed)
{
    if (control < 0 || control >= VirtualControlCount) return;
    virtualControlStates[control] = (isPressed != 0);
}

EMSCRIPTEN_KEEPALIVE
void ResetVirtualControls(void)
{
    for (int i = 0; i < VirtualControlCount; i++) virtualControlStates[i] = false;
}
#endif

static bool IsActionDown(int key, virtual_control_t control)
{
    bool pressed = IsKeyDown(key);

#if defined(PLATFORM_WEB)
    return pressed || virtualControlStates[control];
#else
    return pressed;
#endif
}

static float Clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float GetArtworkLookPitch(
    Vector3 cameraPosition,
    Vector3 forward,
    const Vector3 *artworkPositions,
    const float *artworkWidthCm,
    const float *artworkHeightCm,
    const float *artworkLookLiftFractions,
    const bool *artworkOnZWall,
    int artworkCount,
    float centimetersToWorld,
    float basePitch)
{
    const float fullLookDistance = 0.85f;
    const float maxAutoLookDistance = 2.64f;
    const float lateralFadeMargin = 0.75f;
    const float minimumFacingDot = 0.30f;
    const float minimumWallFacingDot = 0.60f;
    const float minimumProjectedDistance = 0.18f;
    const float maxUpwardPitch = PI/2.8f;
    float bestStrength = 0.0f;
    float bestPitch = basePitch;

    for (int i = 0; i < artworkCount; i++)
    {
        float toArtworkX = artworkPositions[i].x - cameraPosition.x;
        float toArtworkZ = artworkPositions[i].z - cameraPosition.z;
        float horizontalDistance = sqrtf(toArtworkX*toArtworkX + toArtworkZ*toArtworkZ);
        if (horizontalDistance <= 0.001f) continue;

        float facingDot = (toArtworkX*forward.x + toArtworkZ*forward.z)/horizontalDistance;
        if (facingDot <= minimumFacingDot) continue;

        float wallFacingDot = artworkOnZWall[i]
            ? artworkPositions[i].z < 0.0f
                ? -forward.z
                : forward.z
            : artworkPositions[i].x < 0.0f
                ? -forward.x
                : forward.x;
        if (wallFacingDot <= minimumWallFacingDot) continue;

        float pictureWidth = artworkWidthCm[i]*centimetersToWorld;
        float pictureHeight = artworkHeightCm[i]*centimetersToWorld;
        float planeDistance = artworkOnZWall[i]
            ? artworkPositions[i].z < 0.0f
                ? cameraPosition.z - artworkPositions[i].z
                : artworkPositions[i].z - cameraPosition.z
            : artworkPositions[i].x < 0.0f
                ? cameraPosition.x - artworkPositions[i].x
                : artworkPositions[i].x - cameraPosition.x;
        if (planeDistance <= 0.0f || planeDistance >= maxAutoLookDistance) continue;

        float lateralDistance = artworkOnZWall[i]
            ? fabsf(cameraPosition.x - artworkPositions[i].x)
            : fabsf(cameraPosition.z - artworkPositions[i].z);
        float lateralStrength = 1.0f - Clamp01((lateralDistance - pictureWidth*0.5f)/lateralFadeMargin);
        if (lateralStrength <= 0.0f) continue;

        float closeStrength = planeDistance <= fullLookDistance
            ? 1.0f
            : 1.0f - (planeDistance - fullLookDistance)/(maxAutoLookDistance - fullLookDistance);
        closeStrength = Clamp01(closeStrength);
        float facingStrength = Clamp01((facingDot - minimumFacingDot)/(1.0f - minimumFacingDot));
        float wallFacingStrength = Clamp01((wallFacingDot - minimumWallFacingDot)/(1.0f - minimumWallFacingDot));
        float strength = closeStrength
            *(0.45f + 0.55f*lateralStrength)
            *(0.25f + 0.75f*facingStrength)
            *wallFacingStrength;
        if (strength <= bestStrength) continue;

        float topInset = pictureHeight*0.38f;
        float focusY = artworkPositions[i].y + pictureHeight*0.5f - topInset
            + pictureHeight*artworkLookLiftFractions[i];
        float projectedDistance = fmaxf(minimumProjectedDistance, toArtworkX*forward.x + toArtworkZ*forward.z);
        float desiredPitch = atan2f(focusY - cameraPosition.y, projectedDistance);
        if (desiredPitch > maxUpwardPitch) desiredPitch = maxUpwardPitch;

        bestStrength = strength;
        bestPitch = desiredPitch;
    }

    return basePitch + (bestPitch - basePitch)*bestStrength;
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
    float cameraYaw = 0.0f;
    const float cameraTurnSpeed = 1.6f;   // radians per second
    const float cameraMoveSpeed = 2.0f;   // world units per second
    const float cameraLookDrop = 0.15f;
    const float baseLookPitch = atan2f(-cameraLookDrop, 1.0f);
    float cameraPitch = baseLookPitch;
    const float cameraPitchResponsiveness = 7.5f;
    const float centimetersToWorld = 0.016f;

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
    Texture2D wallTexture = CreateWallTexture();
    SetMaterialTexture(&wall.materials[0], MATERIAL_MAP_DIFFUSE, wallTexture);
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
    const Color outsideColor = (Color){ 226, 221, 213, 255 }; // warm light gray beyond the gallery
    const Color cornerColor = (Color){ 72, 76, 82, 255 };    // dark gray corner accents
    const Color smokyAluminumDoorColor = (Color){ 174, 177, 178, 255 }; // floor-adjacent gray with a smoky cast
    const Color northDoorColor = (Color){ 228, 227, 224, 255 };
    const Color windowFrameColor = (Color){ 250, 250, 247, 255 };
    const float pictureY = 1.6f;
    const float pictureDepth = 0.02f;
    const float pictureInset = 0.12f;
    const Color floorColor = (Color){ 182, 184, 180, 255 };  // lighter matte gray
    const bool artworkOnZWall[artworkCount] = {
        false, false, false,
        true, true, true, true, true, true, true,
        false, false, false, false, false, false,
        false,
    };
    const float artworkLookLiftFractions[artworkCount] = {
        0.0f, 0.08f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f,
    };
    const float artwork17Drop = artworkHeightCm[artworkCount - 1]*centimetersToWorld*0.05f;
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
        (Vector3){ -roomHalfX + vestibuleWidth + pictureInset, pictureY - artwork17Drop, roomHalfZ - vestibuleDepth*0.5f },
    };
    const int openingArtworkIndex = artworkCount - 1;
    const float openingYawOffset = -10.0f*DEG2RAD;
    cameraYaw = atan2f(
        artworkPositions[openingArtworkIndex].x - camera.position.x,
        artworkPositions[openingArtworkIndex].z - camera.position.z) + openingYawOffset;
    Vector3 openingForward = { sinf(cameraYaw), 0.0f, cosf(cameraYaw) };
    cameraPitch = GetArtworkLookPitch(
        camera.position,
        openingForward,
        artworkPositions,
        artworkWidthCm,
        artworkHeightCm,
        artworkLookLiftFractions,
        artworkOnZWall,
        artworkCount,
        centimetersToWorld,
        baseLookPitch);
    float openingLookForwardScale = cosf(cameraPitch);
    camera.target = (Vector3){
        camera.position.x + openingForward.x*openingLookForwardScale,
        camera.position.y + sinf(cameraPitch),
        camera.position.z + openingForward.z*openingLookForwardScale
    };
    const Vector3 artworkRotationAxis = (Vector3){ 0.0f, 1.0f, 0.0f };
    const float artworkRotationAngle = 0.0f;
    bool showCorners = true;
#if defined(PLATFORM_WEB)
    bool firstWebFrameReported = false;

    ReportWebLoaderStage("native-assets-loaded");
#endif

    // Main game loop
    while (!WindowShouldClose())
    {
        float frameTime = GetFrameTime();
        Vector3 forward = { sinf(cameraYaw), 0.0f, cosf(cameraYaw) };

        if (IsActionDown(KEY_LEFT, VirtualControlTurnLeft)) cameraYaw += cameraTurnSpeed * frameTime;
        if (IsActionDown(KEY_RIGHT, VirtualControlTurnRight)) cameraYaw -= cameraTurnSpeed * frameTime;

        forward = (Vector3){ sinf(cameraYaw), 0.0f, cosf(cameraYaw) };
        Vector3 right = { -forward.z, 0.0f, forward.x };
        if (IsActionDown(KEY_UP, VirtualControlMoveForward))
        {
            camera.position.x += forward.x * cameraMoveSpeed * frameTime;
            camera.position.z += forward.z * cameraMoveSpeed * frameTime;
        }
        if (IsActionDown(KEY_DOWN, VirtualControlMoveBackward))
        {
            camera.position.x -= forward.x * cameraMoveSpeed * frameTime;
            camera.position.z -= forward.z * cameraMoveSpeed * frameTime;
        }
        if (IsActionDown(KEY_A, VirtualControlStepLeft))
        {
            camera.position.x -= right.x * cameraMoveSpeed * frameTime;
            camera.position.z -= right.z * cameraMoveSpeed * frameTime;
        }
        if (IsActionDown(KEY_D, VirtualControlStepRight))
        {
            camera.position.x += right.x * cameraMoveSpeed * frameTime;
            camera.position.z += right.z * cameraMoveSpeed * frameTime;
        }

        camera.position.y = cameraEyeHeight;
        float desiredPitch = GetArtworkLookPitch(
            camera.position,
            forward,
            artworkPositions,
            artworkWidthCm,
            artworkHeightCm,
            artworkLookLiftFractions,
            artworkOnZWall,
            artworkCount,
            centimetersToWorld,
            baseLookPitch);
        float pitchBlend = 1.0f - expf(-cameraPitchResponsiveness*frameTime);
        cameraPitch += (desiredPitch - cameraPitch)*pitchBlend;
        float lookForwardScale = cosf(cameraPitch);
        Vector3 lookDirection = {
            forward.x*lookForwardScale,
            sinf(cameraPitch),
            forward.z*lookForwardScale
        };
        camera.target = (Vector3){
            camera.position.x + lookDirection.x,
            camera.position.y + lookDirection.y,
            camera.position.z + lookDirection.z
        };
        if (IsKeyPressed(KEY_C)) showCorners = !showCorners;

        BeginDrawing();
        ClearBackground(outsideColor);

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
                artworkRotationAxis,
                artworkRotationAngle,
                pictureScale,
                WHITE);
        }

        DrawGrid(20, 1.0f);
        EndMode3D();

        EndDrawing();

#if defined(PLATFORM_WEB)
        if (!firstWebFrameReported)
        {
            ReportWebLoaderStage("first-frame");
            firstWebFrameReported = true;
        }
#endif
    }

    UnloadModel(wall);
    UnloadModel(door);
    UnloadModel(picture);
    UnloadTexture(wallTexture);
    for (int i = 0; i < artworkCount; i++) UnloadTexture(artworkTextures[i]);

    // De-Initialization
    CloseWindow(); // Close the window and OpenGL context

    return 0;
}
