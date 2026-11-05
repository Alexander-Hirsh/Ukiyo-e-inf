#pragma once

#include <string>
#include <vector>

#include "assets.h"
#include "math_lib.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


/// ##############################################################################################
///                                     Render Constants
/// ##############################################################################################
constexpr uint64_t MAX_POINTS = 4000000;
constexpr int BOUNDS_DEBUG_VERTEX_COUNT = 24;
constexpr int POINT_COMPUTE_LOCAL_SIZE = 256;
constexpr uint64_t MAX_COMPUTE_WORK_GROUP_COUNT_X = 65535;
constexpr int AO_GRID_SIDE_SIZE = 256;
constexpr uint64_t AO_GRID_CELL_COUNT = uint64_t(AO_GRID_SIDE_SIZE) * AO_GRID_SIDE_SIZE * AO_GRID_SIDE_SIZE;
constexpr uint64_t AO_GRID_32BIT_GRID_SIZE = (AO_GRID_CELL_COUNT + 31) / 32;
constexpr uint64_t REDUCTED_AO_GRID_32BIT = (AO_GRID_CELL_COUNT + 7) / 8;

constexpr int RENDER_WIDTH = 1920;
constexpr int RENDER_HEIGHT = 1080;
constexpr int WATER_TEXTURE_WIDTH = 512;
constexpr int WATER_TEXTURE_HEIGHT = 512;
static_assert(MAX_POINTS <= POINT_COMPUTE_LOCAL_SIZE * MAX_COMPUTE_WORK_GROUP_COUNT_X * MAX_COMPUTE_WORK_GROUP_COUNT_X ,
              "MAX_POINTS must fit in one compute dispatch");

constexpr unsigned int MIP_CHAIN_LENGTH = 4;
constexpr int MIP_RATIO = 3;

const char* WATER_DRAW_NAME = "water_plane";
const char* WATER_COMPUTE_NAME = "water_texture";


/// ##############################################################################################
///                                     Render Structs
/// ##############################################################################################
struct Camera3D
{
        float zoom = 1.0f;
        vec3 position;
        float yaw = 0.0f;
        float pitch = 0.5f;
        float distance = 1.0f;

        vec3 fpoint = vec3(0.0, 0.0, 0.0);
        vec3 forward, right, up;

        float fov = 90.0f;
        bool sideScrolling = false;
        float orthographicHeight = 2.0f;

        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        void updateCamera(const Input& input)
        {
                if(!sideScrolling && input.keys[KEY_MOUSE_RIGHT].isDown)
                {
                        constexpr float mouseSensitivity = 0.005f;
                        float maxPitch = degToRad(89.0f);
                        yaw += input.relMousePos.x * mouseSensitivity;
                        pitch -= input.relMousePos.y * mouseSensitivity;
                        if(pitch > maxPitch) pitch = maxPitch;
                        if(pitch < -maxPitch) pitch = -maxPitch;
                }

                float wheelNotches = (float)input.mouseWheelDelta / 120.0f;
                if(sideScrolling)
                {
                        orthographicHeight *= powf(0.9f, wheelNotches);
                        if(orthographicHeight < 0.1f) orthographicHeight = 0.1f;
                        if(orthographicHeight > 100.0f) orthographicHeight = 100.0f;

                        position = fpoint + vec3(0.0f, 0.0f, distance);
                        forward = vec3(0.0f, 0.0f, -1.0f);
                        right = vec3(1.0f, 0.0f, 0.0f);
                        up = vec3(0.0f, 1.0f, 0.0f);
                }
                else
                {
                        distance -= wheelNotches * 0.1f;
                        if(distance < 0.1f) distance = 0.1f;
                        if(distance > 100.0f) distance = 100.0f;

                        float horizontalDistance = distance * cosf(pitch);
                        position = fpoint + vec3(horizontalDistance * sinf(yaw),
                                                 distance * sinf(pitch),
                                                 horizontalDistance * cosf(yaw));
                        forward = normalize(fpoint - position);
                        right = normalize(cross(forward, vec3(0.0f, 1.0f, 0.0f)));
                        up = cross(right, forward);
                }
        }
};

struct RenderData
{
        Camera3D gameCamera;
        int pointCount;
};

struct bloomMip
{
        ivec2 iSize;
        GLuint texture;
};

/// ##############################################################################################
///                                     Simulation Structs
/// ##############################################################################################
struct PARAMETERS
{
        bool wireframe = false;
        GLuint tessLevel = 16;
        int instanceColumns = 1;
        int instanceRows = 1;
        float instanceSpacing = 1.0f;
        constexpr static int maxTessLevel = 64;
};


/// ##############################################################################################
///                                     Render Globals
/// ##############################################################################################
static RenderData* renderData;


/// ##############################################################################################
///                                     Render Functions
/// ##############################################################################################