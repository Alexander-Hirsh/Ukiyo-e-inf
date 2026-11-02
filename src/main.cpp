#include "math_lib.h"

#include "input.h"

#include "platform.h"

#define APIENTRY
#define GL_GLEXT_PROTOTYPES
#include "glcorearb.h"

#include <time.h>

#ifdef _WIN32
#include "win32_platform.cpp"
#endif

#include "gl_renderer.cpp"



/// ##############################################################################################
///                                     Global Variables
/// ##############################################################################################
float deltaTime = 0.0f;
static double minFps = 0.0;
static double maxFps = 0.0;
static double fpsCapped = 0.0;
bool screenshotFrame = false;

/// ##############################################################################################
///                                     Crossplatform Functions
/// ##############################################################################################
#include <chrono>

double getDeltaTime();
void drawParameterGUI(PARAMETERS& params, Camera3D& camera);

int main()
{
        rng_seed(time(NULL), 1);
        getDeltaTime();

        BumpAllocator transientStorage = makeBumpAllocator(MB(2000));
        BumpAllocator persistentStorage = makeBumpAllocator(MB(50));

        input = (Input*)bumpAlloc(&persistentStorage, sizeof(Input));
        renderData = (RenderData*)bumpAlloc(&persistentStorage, sizeof(RenderData));
        *renderData = {};
        renderData->pointCount = MAX_POINTS;

        PARAMETERS params;
        
        platformFillKeycodeLookup();
        if(!platformCreateWindow(RENDER_WIDTH, RENDER_HEIGHT, L"FractalEngine"))
        {
                SM_ERROR("Failed to create platform window");
                return 1;
        }
        input->screenSize.x = RENDER_WIDTH;
        input->screenSize.y = RENDER_HEIGHT;
        
        if(!glInit(&transientStorage)) return 1;

        static float timeSeconds = 0.0f;
        while(running)
        {
                deltaTime = (float)getDeltaTime();
                timeSeconds += deltaTime;

                double currentFps = deltaTime > 0.0f ? 1.0 / deltaTime : 0.0;

                if(currentFps > 0.0)
                {
                        if(minFps == 0.0 || currentFps < minFps) minFps = currentFps;
                        if(currentFps > maxFps) maxFps = currentFps;
                }

                // Performance stats
                {
                        static float displayTimer = 0.0f;
                        displayTimer += deltaTime;
                        if(displayTimer > 1.5f)
                        {
                                displayTimer = 0.0f;
                                fpsCapped = currentFps;
                        }

                        static float resetTimer = 0.0f;
                        resetTimer += deltaTime;
                        if(resetTimer > 10.0f)
                        {
                                resetTimer = 0.0f;
                                minFps = 0.0f;
                                maxFps = 0.0f;
                        }
                }

                platformUpdateWindow();

                static float timer = 0.0f;
                timer += deltaTime;

                renderData->gameCamera.updateCamera(*input);

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplWin32_NewFrame();

                // Draw GUI
                ImGui::NewFrame();
                drawParameterGUI(params, renderData->gameCamera);

                glRender(&transientStorage, params, deltaTime, timeSeconds);

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                platformSwapBuffers();

                transientStorage.used = 0;
        }

        return 0;
}

double getDeltaTime()
{
        // Only executed once when entering the function (static)
        static auto lastTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();

        // seconds
        double delta = std::chrono::duration<double>(currentTime - lastTime).count(); 
        lastTime = currentTime; 

        return delta;
}

void drawParameterGUI(PARAMETERS& params, Camera3D& camera)
{
        ImGui::Begin("Parameters");

        ImGui::SeparatorText("Info");

        ImGui::Text("Ukiyo-e painting generator");
        ImGui::Text("\nFPS | C:%f | N:%f | X:%f", fpsCapped, minFps, maxFps);
        ImGui::Text("MS | %f", 1.0 / deltaTime);

        ImGui::SeparatorText("Buttons");
        
        if(ImGui::Button("Exit")) running = false;
        ImGui::Checkbox("Orthographic side view", &camera.sideScrolling);
        ImGui::Checkbox("Wireframe mode", &params.wireframe);
        ImGui::SliderInt("Tessellation level", (int*)&params.tessLevel, 1, params.maxTessLevel);

        ImGui::SeparatorText("Textures");
        ImGui::Image((void*)(intptr_t)glContext.waterHeightTexture, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();
}