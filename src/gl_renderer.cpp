#include "gl_renderer.h"

#include "render_interface.h"

/// ##############################################################################################
///                                     OpenGL Structs
/// ##############################################################################################
struct GLContext
{
        // Programs
        GLuint waterDrawProgramID;
        GLuint waterComputeProgramID;

        // Buffers
        GLuint VBO;
        GLuint VAO;
        GLuint EBO;
        GLuint waterHeightTexture;

        // Uniforms
        GLuint cameraTransformID;
        GLuint waterComputeTimeID;
        GLuint waterHeightMapID;
        GLuint tessLevelID;

        float vertices[20] = {
                0.5f,  0.0f, 0.5f,  1.0f, 1.0f,  // top right
                -0.5f, 0.0f, 0.5f,  0.0f, 1.0f,  // top left
                0.5f,  0.0f, -0.5f, 1.0f, 0.0f,  // bottom right
                -0.5f, 0.0f, -0.5f, 0.0f, 0.0f   // bottom left
        };
        unsigned int indices[6] = {
                0, 1, 3,   // first triangle
                1, 2, 3    // second triangle
        };  

        long long shaderTimestamps[30] = { 0 };
        std::string shaderPathes[30];
        unsigned int shaderCount = 0;
};


/// ##############################################################################################
///                                     OpenGl Globals
/// ##############################################################################################
static GLContext glContext;


/// ##############################################################################################
///                                     OpenGL Functions
/// ##############################################################################################
static void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar* message, const void* user)
{
        if(
        //severity == GL_DEBUG_SEVERITY_LOW || 
        // severity == GL_DEBUG_SEVERITY_MEDIUM ||
        severity == GL_DEBUG_SEVERITY_HIGH)
        {
                SM_ASSERT(false, "OpenGL Error: %s", message);
        }
        else
        {
                SM_TRACE((char*)message);
        }
}

GLuint glShaderInit(int shaderType, const char* shaderPath, BumpAllocator* transientStorage)
{
        int fileSize = 0;
        char* shaderData = readFile(shaderPath, &fileSize, transientStorage);
        if(!shaderData)
        {
                SM_ASSERT(false, "Failed to load %s shader", shaderPath);
                return 0;
        }

        GLuint shaderID = glCreateShader(shaderType);
        glShaderSource(shaderID, 1, &shaderData, 0);
        glCompileShader(shaderID);

        glContext.shaderPathes[glContext.shaderCount] = shaderPath;
        glContext.shaderTimestamps[glContext.shaderCount++] = getTimestamp(shaderPath);

        // Check if shader actually compiled
        {
                int success;
                char shaderLog[2048] = {};

                glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
                if(!success)
                {
                        glGetShaderInfoLog(shaderID, 2048, 0, shaderLog);
                        SM_ASSERT(false, "Failed to compile %s Shader, Error: %s", shaderPath, shaderLog);
                        glDeleteShader(shaderID);
                        return 0;
                }
        }

        return shaderID;
}

static const char* extensionFor(GLenum type)
{
    switch (type) {
        case GL_VERTEX_SHADER:          return ".vert";
        case GL_TESS_CONTROL_SHADER:    return ".tesc";
        case GL_TESS_EVALUATION_SHADER: return ".tese";
        case GL_GEOMETRY_SHADER:        return ".geom";
        case GL_FRAGMENT_SHADER:        return ".frag";
        case GL_COMPUTE_SHADER:         return ".comp";
        default:                        return "";
    }
}

GLuint glCreateShaderProgram(BumpAllocator* transientStorage, const char* shaderName, std::initializer_list<GLenum> shaderTypes)
{
        GLuint glProgramID = glCreateProgram();
        std::vector<GLuint> compiled;

        for (GLenum type : shaderTypes) {
                GLuint glShaderID = glShaderInit(type, 
                                                ("shaders/" + std::string(shaderName) + std::string(extensionFor(type))).c_str(),
                                                 transientStorage);
                if (!glShaderID) 
                {                           // clean up and bail out
                        for (GLuint c : compiled) glDeleteShader(c);
                                glDeleteProgram(glProgramID);
                                return 0;
                }
                glAttachShader(glProgramID, glShaderID);
                compiled.push_back(glShaderID);
        }

        glLinkProgram(glProgramID);

        GLint success = GL_FALSE;
        glGetProgramiv(glProgramID, GL_LINK_STATUS, &success);
        if(!success)
        {
                char programInfoLog[512] = {};
                glGetProgramInfoLog(glProgramID, sizeof(programInfoLog), 0, programInfoLog);
                SM_ASSERT(false, "Failed to link shader program: %s", programInfoLog);
                glDeleteProgram(glProgramID);
                return 0;
        }

        return glProgramID;
}

bool glCreateALLShaderPrograms(BumpAllocator* transientStorage)
{
        glContext.shaderCount = 0;
        
        if(glContext.waterDrawProgramID) glDeleteProgram(glContext.waterDrawProgramID);
        glContext.waterDrawProgramID = glCreateShaderProgram(transientStorage,
                                                             WATER_DRAW_NAME, 
                                                             {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER});

        if(glContext.waterComputeProgramID) glDeleteProgram(glContext.waterComputeProgramID);
        glContext.waterComputeProgramID = glCreateShaderProgram(transientStorage,
                                                                WATER_COMPUTE_NAME,
                                                                {GL_COMPUTE_SHADER});

        if(!glContext.waterDrawProgramID || !glContext.waterComputeProgramID) return false;

        return true;
}

bool glInit(BumpAllocator* transientStorage)
{
        loadGlFunctions();

        glDebugMessageCallback(&glDebugCallback, nullptr);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glEnable(GL_DEBUG_OUTPUT);

        glCreateALLShaderPrograms(transientStorage);

        glGenTextures(1, &glContext.waterHeightTexture);
        glBindTexture(GL_TEXTURE_2D, glContext.waterHeightTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WATER_TEXTURE_WIDTH, WATER_TEXTURE_HEIGHT,
                     0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        // glGenVertexArrays(1, &glContext.pointVertexArrayID);
        // glBindVertexArray(glContext.pointVertexArrayID);

        glGenBuffers(1, &glContext.VBO);
        glGenVertexArrays(1, &glContext.VAO);
        glGenBuffers(1, &glContext.EBO);
        
        glBindVertexArray(glContext.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, glContext.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glContext.vertices), glContext.vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glContext.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glContext.indices), glContext.indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);  

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Uniforms
        glContext.cameraTransformID = glGetUniformLocation(glContext.waterDrawProgramID, "cameraTransform");
        glContext.waterHeightMapID = glGetUniformLocation(glContext.waterDrawProgramID, "heightMap");
        glContext.waterComputeTimeID = glGetUniformLocation(glContext.waterComputeProgramID, "timeSeconds");
        glContext.tessLevelID = glGetUniformLocation(glContext.waterDrawProgramID, "tessLevel");

        glEnable(GL_FRAMEBUFFER_SRGB);
        glPatchParameteri(GL_PATCH_VERTICES, 4);
        // glEnable(GL_PROGRAM_POINT_SIZE);

        return true;
}

void glRender(BumpAllocator* transientStorage, PARAMETERS params, float deltaTime, float timeSeconds)
{
        // HOT SHADER RELOADING
        {
                for(int sID = 0; sID < glContext.shaderCount; sID++)
                {
                        long long cTimestamp = getTimestamp(glContext.shaderPathes[sID].c_str());

                        if(cTimestamp > glContext.shaderTimestamps[sID])
                        {
                                glCreateALLShaderPrograms(transientStorage);
                                break;
                        }
                }
        }

        Camera3D& camera = renderData->gameCamera;

        float aspectRatio = (float)input->screenSize.x / input->screenSize.y;
        mat4 cameraSpaceTransform = {};

        if(camera.sideScrolling)
        {
                float halfHeight = camera.orthographicHeight * 0.5f;
                float halfWidth = halfHeight * aspectRatio;
                mat4 orthoProjection = orthographicProjection3D(-halfWidth, halfWidth,
                                                                 -halfHeight, halfHeight,
                                                                 camera.nearPlane, camera.farPlane);
                vec3 sideViewPosition = camera.fpoint + vec3(0.0f, 0.0f, camera.distance);
                mat4 view = lookAt(vec3(0.0f, 0.0f, -1.0f),
                                   vec3(1.0f, 0.0f, 0.0f),
                                   vec3(0.0f, 1.0f, 0.0f)) *
                            translationMatrix(sideViewPosition * -1.0f);
                cameraSpaceTransform = orthoProjection * view;
        }
        else
        {
                float top = camera.nearPlane * tanf(degToRad(camera.fov / 2.0f));
                float halfWidth = top * aspectRatio;
                mat4 perspProjection = perspectiveProjection(-halfWidth, halfWidth, top, -top,
                                                              camera.nearPlane, camera.farPlane);
                mat4 view = lookAt(camera.forward, camera.right * -1.0f, camera.up * -1.0f) *
                            translationMatrix(camera.position * -1.0f);
                cameraSpaceTransform = perspProjection * view;
        }

        glViewport(0, 0, input->screenSize.x, input->screenSize.y);
        glClearColor(180.0/255.0, 160.0/255.0, 131.0/255.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);


        // Use compute program for texture
        {
                glUseProgram(glContext.waterComputeProgramID);
                glUniform1f(glContext.waterComputeTimeID, timeSeconds);
                glBindImageTexture(0, glContext.waterHeightTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glDispatchCompute((WATER_TEXTURE_WIDTH + 7) / 8, (WATER_TEXTURE_HEIGHT + 7) / 8, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        // Use water draw program
        {        
                glUseProgram(glContext.waterDrawProgramID);

                glUniform1i(glContext.tessLevelID, params.tessLevel);
                glUniformMatrix4fv(glContext.cameraTransformID, 1, GL_FALSE, &cameraSpaceTransform.ax);
                glUniform1i(glContext.waterHeightMapID, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, glContext.waterHeightTexture);

                glBindVertexArray(glContext.VAO);

                glPolygonMode(GL_FRONT_AND_BACK, params.wireframe ? GL_LINE : GL_FILL);

                glDrawArrays(GL_PATCHES, 0, 4);
                
        }
}

void takeScreenshot(GLuint FBOID, std::string timestamp)
{
        std::vector<unsigned char> pixels(RENDER_WIDTH * RENDER_HEIGHT * 4);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);   // the RGBA8 one
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, RENDER_WIDTH, RENDER_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // OpenGL's origin is bottom-left, PNG's is top-left
        stbi_flip_vertically_on_write(1);
        std::string fileName = "screenshots/" + timestamp + "capture.png";
        stbi_write_png(fileName.c_str(), RENDER_WIDTH, RENDER_HEIGHT, 4, pixels.data(), RENDER_WIDTH * 4);
}