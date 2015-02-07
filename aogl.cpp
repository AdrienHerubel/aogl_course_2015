#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include <cmath>

#include "glew/glew.h"

#include "GLFW/glfw3.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr
#include "glm/ext.hpp"

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    

// Shader utils
int check_compile_error(GLuint shader, const char ** sourceBuffer);
int check_link_error(GLuint program);
GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize);
GLuint compile_shader_from_file(GLenum shaderType, const char * fileName);

// OpenGL utils
bool checkError(const char* title);

struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};
void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    int scroll;
    bool display;
    bool blit;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;
void init_gui_states(GUIStates & guiStates);

struct SpotLight
{
    glm::vec3 position;
    float angle;
    glm::vec3 direction;
    float penumbraAngle;
    glm::vec3 color;
    float intensity;
    glm::mat4 worldToLightScreen;
};

int main( int argc, char **argv )
{
    int width = 1024, height= 768;
    float widthf = (float) width, heightf = (float) height;
    double t;
    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    int const DPI = 2; // For retina screens only
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    int const DPI = 1;
# endif

    // Open a window and create its OpenGL context
    GLFWwindow * window = glfwCreateWindow(width/DPI, height/DPI, "aogl", 0, 0);
    if( ! window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    glfwMakeContextCurrent(window);

    // Init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode( window, GLFW_STICKY_KEYS, GL_TRUE );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    // Init viewer structures
    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);
    float instanceCount = 2500;
    float pointLightCount = 10;
    float directionalLightCount = 1;
    float spotLightCount = 1;
    float speed = 1.0;

    // Load images and upload textures
    GLuint textures[2];
    glGenTextures(2, textures);
    int x;
    int y;
    int comp;

    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    fprintf(stderr, "Diffuse %dx%d:%d\n", x, y, comp);

    unsigned char * spec = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, spec);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    fprintf(stderr, "Spec %dx%d:%d\n", x, y, comp);
    checkError("Texture Initialization");

    // Try to load and compile blit shaders
    GLuint vertBlitShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "blit.vert");
    GLuint fragBlitShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "blit.frag");
    GLuint blitProgramObject = glCreateProgram();
    glAttachShader(blitProgramObject, vertBlitShaderId);
    glAttachShader(blitProgramObject, fragBlitShaderId);
    glLinkProgram(blitProgramObject);
    if (check_link_error(blitProgramObject) < 0)
        exit(1);

    // Try to load and compile gbuffer shaders
    GLuint vertgbufferShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "gbuffer.vert");
    GLuint fraggbufferShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "gbuffer.frag");
    GLuint gbufferProgramObject = glCreateProgram();
    glAttachShader(gbufferProgramObject, vertgbufferShaderId);
    glAttachShader(gbufferProgramObject, fraggbufferShaderId);
    glLinkProgram(gbufferProgramObject);
    if (check_link_error(gbufferProgramObject) < 0)
        exit(1);

    // Try to load and compile shadow shaders
    GLuint fragshadowShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "shadow.frag");
    GLuint shadowProgramObject = glCreateProgram();
    glAttachShader(shadowProgramObject, vertgbufferShaderId);
    glAttachShader(shadowProgramObject, fragshadowShaderId);
    glLinkProgram(shadowProgramObject);
    if (check_link_error(shadowProgramObject) < 0)
        exit(1);

    // Try to load and compile pointlight shaders
    GLuint fragpointlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "pointlight.frag");
    GLuint pointlightProgramObject = glCreateProgram();
    glAttachShader(pointlightProgramObject, vertBlitShaderId);
    glAttachShader(pointlightProgramObject, fragpointlightShaderId);
    glLinkProgram(pointlightProgramObject);
    if (check_link_error(pointlightProgramObject) < 0)
        exit(1);

    // Try to load and compile directionallight shaders
    GLuint fragdirectionallightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "directionallight.frag");
    GLuint directionallightProgramObject = glCreateProgram();
    glAttachShader(directionallightProgramObject, vertBlitShaderId);
    glAttachShader(directionallightProgramObject, fragdirectionallightShaderId);
    glLinkProgram(directionallightProgramObject);
    if (check_link_error(directionallightProgramObject) < 0)
        exit(1);

    // Try to load and compile spotlight shaders
    GLuint fragspotlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "spotlight.frag");
    GLuint spotlightProgramObject = glCreateProgram();
    glAttachShader(spotlightProgramObject, vertBlitShaderId);
    glAttachShader(spotlightProgramObject, fragspotlightShaderId);
    glLinkProgram(spotlightProgramObject);
    if (check_link_error(spotlightProgramObject) < 0)
        exit(1);

    // Upload uniforms
    GLuint mvpLocation = glGetUniformLocation(gbufferProgramObject, "MVP");
    GLuint mvLocation = glGetUniformLocation(gbufferProgramObject, "MV");
    GLuint timeLocation = glGetUniformLocation(gbufferProgramObject, "Time");
    GLuint diffuseLocation = glGetUniformLocation(gbufferProgramObject, "Diffuse");
    GLuint specLocation = glGetUniformLocation(gbufferProgramObject, "Specular");
    GLuint specularPowerLocation = glGetUniformLocation(gbufferProgramObject, "SpecularPower");
    GLuint instanceCountLocation = glGetUniformLocation(gbufferProgramObject, "InstanceCount");
    GLuint blitTextureLocation = glGetUniformLocation(blitProgramObject, "Texture");
    glProgramUniform1i(blitProgramObject, blitTextureLocation, 0);
 
    GLuint pointlightColorLocation = glGetUniformLocation(pointlightProgramObject, "ColorBuffer");
    GLuint pointlightNormalLocation = glGetUniformLocation(pointlightProgramObject, "NormalBuffer");
    GLuint pointlightDepthLocation = glGetUniformLocation(pointlightProgramObject, "DepthBuffer");
    GLuint pointlightLightLocation = glGetUniformBlockIndex(pointlightProgramObject, "light");
    GLuint pointInverseProjectionLocation = glGetUniformLocation(pointlightProgramObject, "InverseProjection");
    glProgramUniform1i(pointlightProgramObject, pointlightColorLocation, 0);
    glProgramUniform1i(pointlightProgramObject, pointlightNormalLocation, 1);
    glProgramUniform1i(pointlightProgramObject, pointlightDepthLocation, 2);

    GLuint directionallightColorLocation = glGetUniformLocation(directionallightProgramObject, "ColorBuffer");
    GLuint directionallightNormalLocation = glGetUniformLocation(directionallightProgramObject, "NormalBuffer");
    GLuint directionallightDepthLocation = glGetUniformLocation(directionallightProgramObject, "DepthBuffer");
    GLuint directionallightLightLocation = glGetUniformBlockIndex(directionallightProgramObject, "light");
    GLuint directionalInverseProjectionLocation = glGetUniformLocation(directionallightProgramObject, "InverseProjection");
    glProgramUniform1i(directionallightProgramObject, directionallightColorLocation, 0);
    glProgramUniform1i(directionallightProgramObject, directionallightNormalLocation, 1);
    glProgramUniform1i(directionallightProgramObject, directionallightDepthLocation, 2);

    GLuint spotlightColorLocation = glGetUniformLocation(spotlightProgramObject, "ColorBuffer");
    GLuint spotlightNormalLocation = glGetUniformLocation(spotlightProgramObject, "NormalBuffer");
    GLuint spotlightDepthLocation = glGetUniformLocation(spotlightProgramObject, "DepthBuffer");
    GLuint spotlightShadowLocation = glGetUniformLocation(spotlightProgramObject, "Shadow");
    GLuint spotlightLightLocation = glGetUniformBlockIndex(spotlightProgramObject, "light");
    GLuint spotInverseProjectionLocation = glGetUniformLocation(spotlightProgramObject, "InverseProjection");
    glProgramUniform1i(spotlightProgramObject, spotlightColorLocation, 0);
    glProgramUniform1i(spotlightProgramObject, spotlightNormalLocation, 1);
    glProgramUniform1i(spotlightProgramObject, spotlightDepthLocation, 2);
    glProgramUniform1i(spotlightProgramObject, spotlightShadowLocation, 3);


    GLuint shadowMVPLocation = glGetUniformLocation(shadowProgramObject, "MVP");
    GLuint shadowMVLocation = glGetUniformLocation(shadowProgramObject, "MV");
    GLuint shadowTimeLocation = glGetUniformLocation(shadowProgramObject, "Time");
    GLuint shadowInstanceCountLocation = glGetUniformLocation(shadowProgramObject, "InstanceCount");

   if (!checkError("Uniforms"))
        exit(1);

    // Load geometry
    int cube_triangleCount = 12;
    int cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };
    int plane_triangleCount = 2;
    int plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 50.f, 50.f, 0.f, 50.f, 50.f};
    float plane_vertices[] = {-50.0, -1.0, 50.0, 50.0, -1.0, 50.0, -50.0, -1.0, -50.0, 50.0, -1.0, -50.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_vertices[] =  {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};

    // Vertex Array Object
    GLuint vao[3];
    glGenVertexArrays(3, vao);

    // Vertex Buffer Objects
    GLuint vbo[10];
    glGenBuffers(10, vbo);

    // Cube
    glBindVertexArray(vao[0]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);

    // Plane
    glBindVertexArray(vao[1]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);

    // Quad
    glBindVertexArray(vao[2]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Unbind everything. Potentially illegal on some implementations
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Update and bind uniform buffer object
    const int SPOT_LIGHT_COUNT = 16;
    GLuint ubo[1];
    glGenBuffers(1, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
    GLint uboOffset = 0;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboOffset);
    GLint uboSize = 0;
    glGetActiveUniformBlockiv(spotlightProgramObject, spotlightLightLocation, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);
    uboSize = ((uboSize / uboOffset) + 1) * uboOffset;
    glBufferData(GL_UNIFORM_BUFFER, uboSize * SPOT_LIGHT_COUNT, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Init frame buffers
    GLuint gbufferFbo;
    GLuint gbufferTextures[3];
    GLuint gbufferDrawBuffers[2];
    glGenTextures(3, gbufferTextures);

    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create normal texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, gbufferDrawBuffers);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, gbufferTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D, gbufferTextures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferTextures[2], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building gbuffer framebuffer\n");
        exit( EXIT_FAILURE );
    }

    // Create shadow FBO
    GLuint shadowFbo;
    glGenFramebuffers(1, &shadowFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
    
    // Create shadow textures
    const int SPOT_LIGHT_SHADOW_RES = 1024;
    GLuint shadowTextures[SPOT_LIGHT_COUNT];
    glGenTextures(SPOT_LIGHT_COUNT, shadowTextures);
    for (int i = 0; i < SPOT_LIGHT_COUNT; ++i) 
    {
        glBindTexture(GL_TEXTURE_2D, shadowTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SPOT_LIGHT_SHADOW_RES, SPOT_LIGHT_SHADOW_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
#if 1        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,  GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,  GL_LEQUAL);
#endif        
    }

    // Create a render buffer since we don't need to read shadow color 
    // in a texture
    GLuint shadowRenderBuffer;
    glGenRenderbuffers(1, &shadowRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, shadowRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, SPOT_LIGHT_SHADOW_RES, SPOT_LIGHT_SHADOW_RES);

    // Attach the first texture to the depth attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTextures[0], 0);

    // Attach the renderbuffer
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, shadowRenderBuffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building shadow framebuffer\n");
        exit( EXIT_FAILURE );
    }

    // Fall back to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    do
    {
        t = glfwGetTime() * speed;

        // Mouse states
        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

        // Camera movements
        int altPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            double x; double y;
            glfwGetCursorPos(window, &x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }
        if (glfwGetKey(window, GLFW_KEY_G))
            guiStates.display = ! guiStates.display;
        if (glfwGetKey(window, GLFW_KEY_B))
            guiStates.blit = ! guiStates.blit;

// static void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods)
// {
//     if (key == GLFW_KEY_G && action == GLFW_PRESS)
//         g_guiStates->display = ! g_guiStates->display;
//     if (key == GLFW_KEY_B && action == GLFW_PRESS)
//         g_guiStates->blit = ! g_guiStates->blit;
// }

        // Default states
        glEnable(GL_DEPTH_TEST);

        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Viewport 
        glViewport( 0, 0, width, height  );

        // Bind gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);

        // Clear the gbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 100.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;
        glm::mat4 mv = worldToView * objectToWorld;
        glm::mat4 mvp = projection * mv;
        //glm::mat4 inverseProjection = glm::transpose(glm::inverse(projection));
        glm::mat4 inverseProjection = glm::inverse(projection);

        // Select textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);

        // Select shader
        glUseProgram(gbufferProgramObject);

        // Upload uniforms
        glProgramUniformMatrix4fv(gbufferProgramObject, mvpLocation, 1, 0, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(gbufferProgramObject, mvLocation, 1, 0, glm::value_ptr(mv));
        glProgramUniform1i(gbufferProgramObject, instanceCountLocation, (int) instanceCount);
        glProgramUniform1f(gbufferProgramObject, specularPowerLocation, 30.f);
        glProgramUniform1f(gbufferProgramObject, timeLocation, t);
        glProgramUniform1i(gbufferProgramObject, diffuseLocation, 0);
        glProgramUniform1i(gbufferProgramObject, specLocation, 1);
        glProgramUniformMatrix4fv(pointlightProgramObject, pointInverseProjectionLocation, 1, 0, glm::value_ptr(inverseProjection));
        glProgramUniformMatrix4fv(directionallightProgramObject, directionalInverseProjectionLocation, 1, 0, glm::value_ptr(inverseProjection));
        glProgramUniformMatrix4fv(spotlightProgramObject, spotInverseProjectionLocation, 1, 0, glm::value_ptr(inverseProjection));
        glProgramUniform1i(shadowProgramObject, shadowInstanceCountLocation, (int) instanceCount);

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, (int) instanceCount);
        //glDrawElements(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        glProgramUniform1f(gbufferProgramObject, timeLocation, 0.f);
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Shadow passes
        // Map the spot light data UBO
        glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
        char * spotLightBuffer = (char *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, uboSize * SPOT_LIGHT_COUNT, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        // Bind the shadow FBO
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
        // Use shadow program
        glUseProgram(shadowProgramObject);
        glViewport(0, 0, SPOT_LIGHT_SHADOW_RES, SPOT_LIGHT_SHADOW_RES);
        for (int i = 0; i < spotLightCount; ++i)
        {
            // Setup light data
#if 1                
            glm::vec3 lp(i*2, 5.f, i*2);
            glm::vec3 ld(-1.f, -1.f, 0.f);
            float angle = 45.f;
            float penumbraAngle = 50.f;
            glm::vec3 color(1.f, 1.f, 1.f); 
#else
            float iF = (float) i +1.f;
            glm::vec3 lp((spotLightCount*2.f*sinf(t)) * cosf(t/iF), 5.f + sinf(t / iF), fabsf(spotLightCount*2.f*cosf(t)) * sinf(t/iF));
            glm::vec3 ld(sinf(t*10.0+i), -1.0, 0.0);
            float angle = 45.f + 20.f * cos(t + i);
            float penumbraAngle = 60.f + 20.f * cos(t + i);
            glm::vec3 color(fabsf(cos(t+i*2.f)), 1.-fabsf(sinf(t+i)), 0.5f + 0.5f-fabsf(cosf(t+i))); 
#endif            
            // Light space matrices
            glm::mat4 projection = glm::perspective(glm::radians(penumbraAngle*2.f), 1.f, 1.f, 100.f); 
            glm::mat4 worldToLight = glm::lookAt(lp, lp + ld, glm::vec3(0.f, 0.f, -1.f));
            glm::mat4 objectToWorld;
            glm::mat4 objectToLight = worldToLight * objectToWorld;
            glm::mat4 objectToLightScreen = projection * objectToLight;
            SpotLight s = { 
                glm::vec3( worldToView * glm::vec4(lp, 1.f)), angle,
                glm::vec3( worldToView * glm::vec4(ld, 0.f)), penumbraAngle,
                color, 30.f, 
                projection * worldToLight * glm::inverse(worldToView)
            };
            *((SpotLight *) (spotLightBuffer + i * uboSize)) = s;
            //std::cout << glm::to_string(worldToLight) << std::endl;
            // Attach shadow texture for current light
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTextures[i], 0);
            // Clear only the depth buffer
            glClear(GL_DEPTH_BUFFER_BIT);


            // Update scene uniforms
            glProgramUniformMatrix4fv(shadowProgramObject, shadowMVPLocation, 1, 0, glm::value_ptr(objectToLightScreen));
            glProgramUniformMatrix4fv(shadowProgramObject, shadowMVLocation, 1, 0, glm::value_ptr(objectToLight));

            // Render the scene
            glProgramUniform1f(shadowProgramObject, shadowTimeLocation, t);
            glBindVertexArray(vao[0]);
            glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, (int) instanceCount);
            glProgramUniform1f(shadowProgramObject, shadowTimeLocation, 0.f);
            glBindVertexArray(vao[1]);
            glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }        
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        // Select textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
        glActiveTexture(GL_TEXTURE3);

        // Bind the same VAO for all lights
        glBindVertexArray(vao[2]);
        // Render spot lights
        glUseProgram(spotlightProgramObject);
        for (int i = 0; i < spotLightCount; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, shadowTextures[i]);
            glBindBufferRange(GL_UNIFORM_BUFFER, spotlightLightLocation, ubo[0], uboSize * i, uboSize);
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }    

        glDisable(GL_BLEND);

        if (guiStates.blit)
        {
            // Bind blit shader
            glUseProgram(blitProgramObject);
            // Upload uniforms
            // use only unit 0
            glActiveTexture(GL_TEXTURE0);
            // Bind VAO
            glBindVertexArray(vao[2]);

            // Viewport 
            glViewport( 0, 0, width/4, height/4  );
            // Bind texture
            glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            // Viewport 
            glViewport( width/4, 0, width/4, height/4  );
            // Bind texture
            glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            // Viewport 
            glViewport( width/4 * 2, 0, width/4, height/4  );
            // Bind texture
            glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            // Viewport 
            glViewport( width/4 * 3, 0, SPOT_LIGHT_SHADOW_RES / 5, SPOT_LIGHT_SHADOW_RES / 5  );
            // Bind texture
            glBindTexture(GL_TEXTURE_2D, shadowTextures[((int) t) % (int) spotLightCount]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }

        if (guiStates.display)
        {
            // Draw UI
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, width, height);

            unsigned char mbut = 0;
            int mscroll = 0;
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            mousex*=DPI;
            mousey*=DPI;
            mousey = height - mousey;

            if( leftButton == GLFW_PRESS )
                mbut |= IMGUI_MBUT_LEFT;

            imguiBeginFrame(mousex, mousey, mbut, mscroll);
            char lineBuffer[512];
            imguiBeginScrollArea("aogl", width - 210, height - 310, 200, 300, &guiStates.scroll);
            sprintf(lineBuffer, "FPS %f", fps);
            imguiLabel(lineBuffer);
            imguiSlider("Speed", &speed, 0.0, 1.0, 0.01);
            imguiSlider("Point Lights", &pointLightCount, 0.0, 100.0, 1);
            imguiSlider("Directional Lights", &directionalLightCount, 0.0, 100.0, 1);
            imguiSlider("Spot Lights", &spotLightCount, 1.0, 16.0, 1);

            imguiEndScrollArea();
            imguiEndFrame();
            imguiRenderGLDraw(width, height);

            glDisable(GL_BLEND);
        }
        // Check for errors
        checkError("End loop");

        glfwSwapBuffers(window);
        glfwPollEvents();

        double newTime = glfwGetTime();
        fps = 1.f/ (newTime - t);
    } // Check if the ESC key was pressed
    while( glfwGetKey( window, GLFW_KEY_ESCAPE ) != GLFW_PRESS );

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

// No windows implementation of strsep
char * strsep_custom(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    return 0;
}

int check_compile_error(GLuint shader, const char ** sourceBuffer)
{
    // Get error log size and print it eventually
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        char *token, *string;
        string = strdup(sourceBuffer[0]);
        int lc = 0;
        while ((token = strsep_custom(&string, "\n")) != NULL) {
           printf("%3d : %s\n", lc, token);
           ++lc;
        }
        fprintf(stderr, "Compile : %s", log);
        delete[] log;
    }
    // If an error happend quit
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
        return -1;     
    return 0;
}

int check_link_error(GLuint program)
{
    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        fprintf(stderr, "Link : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;
    return 0;
}

GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize)
{
    GLuint shaderObject = glCreateShader(shaderType);
    const char * sc[1] = { sourceBuffer };
    glShaderSource(shaderObject, 
                   1, 
                   sc,
                   NULL);
    glCompileShader(shaderObject);
    check_compile_error(shaderObject, sc);
    return shaderObject;
}

GLuint compile_shader_from_file(GLenum shaderType, const char * path)
{
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return 0;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    size_t t = fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    GLuint shaderObject = compile_shader(shaderType, buffer, fileSize );
    delete[] buffer;
    if (t != fileSize)
        return -1;
    return shaderObject;
}


bool checkError(const char* title)
{
    int error;
    if((error = glGetError()) != GL_NO_ERROR)
    {
        std::string errorString;
        switch(error)
        {
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        default:
            errorString = "UNKNOWN";
            break;
        }
        fprintf(stdout, "OpenGL Error(%s): %s\n", errorString.c_str(), title);
    }
    return error == GL_NO_ERROR;
}

void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
    guiStates.display = false;
    guiStates.blit = false;
    guiStates.scroll = 0;
}
