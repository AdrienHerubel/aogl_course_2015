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
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;
void init_gui_states(GUIStates & guiStates);




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
    float pointLightCount = 0;
    float directionalLightCount = 1;
    float spotLightCount = 0;
    float speed = 1.f;
    float gamma = 1.f;
    float factor = 1.f;
    float sampleCount = 3.0;

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
    GLuint blitTextureLocation = glGetUniformLocation(blitProgramObject, "Texture");
    glProgramUniform1i(blitProgramObject, blitTextureLocation, 0);

    // Try to load and compile gbuffer shaders
    GLuint vertgbufferShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "gbuffer.vert");
    GLuint fraggbufferShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "gbuffer.frag");
    GLuint gbufferProgramObject = glCreateProgram();
    glAttachShader(gbufferProgramObject, vertgbufferShaderId);
    glAttachShader(gbufferProgramObject, fraggbufferShaderId);
    glLinkProgram(gbufferProgramObject);
    if (check_link_error(gbufferProgramObject) < 0)
        exit(1);
    GLuint mvpLocation = glGetUniformLocation(gbufferProgramObject, "MVP");
    GLuint mvLocation = glGetUniformLocation(gbufferProgramObject, "MV");
    GLuint timeLocation = glGetUniformLocation(gbufferProgramObject, "Time");
    GLuint diffuseLocation = glGetUniformLocation(gbufferProgramObject, "Diffuse");
    GLuint specLocation = glGetUniformLocation(gbufferProgramObject, "Specular");
    GLuint lightLocation = glGetUniformLocation(gbufferProgramObject, "Light");
    GLuint specularPowerLocation = glGetUniformLocation(gbufferProgramObject, "SpecularPower");
    GLuint instanceCountLocation = glGetUniformLocation(gbufferProgramObject, "InstanceCount");
    glProgramUniform1i(gbufferProgramObject, diffuseLocation, 0);
    glProgramUniform1i(gbufferProgramObject, specLocation, 1);

    // Try to load and compile pointlight shaders
    GLuint fragpointlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "pointlight.frag");
    GLuint pointlightProgramObject = glCreateProgram();
    glAttachShader(pointlightProgramObject, vertBlitShaderId);
    glAttachShader(pointlightProgramObject, fragpointlightShaderId);
    glLinkProgram(pointlightProgramObject);
    if (check_link_error(pointlightProgramObject) < 0)
        exit(1);
    GLuint pointlightColorLocation = glGetUniformLocation(pointlightProgramObject, "ColorBuffer");
    GLuint pointlightNormalLocation = glGetUniformLocation(pointlightProgramObject, "NormalBuffer");
    GLuint pointlightDepthLocation = glGetUniformLocation(pointlightProgramObject, "DepthBuffer");
    GLuint pointlightLightLocation = glGetUniformBlockIndex(pointlightProgramObject, "light");
    GLuint pointInverseProjectionLocation = glGetUniformLocation(pointlightProgramObject, "InverseProjection");
    glProgramUniform1i(pointlightProgramObject, pointlightColorLocation, 0);
    glProgramUniform1i(pointlightProgramObject, pointlightNormalLocation, 1);
    glProgramUniform1i(pointlightProgramObject, pointlightDepthLocation, 2);

    // Try to load and compile directionallight shaders
    GLuint fragdirectionallightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "directionallight.frag");
    GLuint directionallightProgramObject = glCreateProgram();
    glAttachShader(directionallightProgramObject, vertBlitShaderId);
    glAttachShader(directionallightProgramObject, fragdirectionallightShaderId);
    glLinkProgram(directionallightProgramObject);
    if (check_link_error(directionallightProgramObject) < 0)
        exit(1);
    GLuint directionallightColorLocation = glGetUniformLocation(directionallightProgramObject, "ColorBuffer");
    GLuint directionallightNormalLocation = glGetUniformLocation(directionallightProgramObject, "NormalBuffer");
    GLuint directionallightDepthLocation = glGetUniformLocation(directionallightProgramObject, "DepthBuffer");
    GLuint directionallightLightLocation = glGetUniformBlockIndex(directionallightProgramObject, "light");
    GLuint directionalInverseProjectionLocation = glGetUniformLocation(directionallightProgramObject, "InverseProjection");
    glProgramUniform1i(directionallightProgramObject, directionallightColorLocation, 0);
    glProgramUniform1i(directionallightProgramObject, directionallightNormalLocation, 1);
    glProgramUniform1i(directionallightProgramObject, directionallightDepthLocation, 2);

    // Try to load and compile spotlight shaders
    GLuint fragspotlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "spotlight.frag");
    GLuint spotlightProgramObject = glCreateProgram();
    glAttachShader(spotlightProgramObject, vertBlitShaderId);
    glAttachShader(spotlightProgramObject, fragspotlightShaderId);
    glLinkProgram(spotlightProgramObject);
    if (check_link_error(spotlightProgramObject) < 0)
        exit(1);
    GLuint spotlightColorLocation = glGetUniformLocation(spotlightProgramObject, "ColorBuffer");
    GLuint spotlightNormalLocation = glGetUniformLocation(spotlightProgramObject, "NormalBuffer");
    GLuint spotlightDepthLocation = glGetUniformLocation(spotlightProgramObject, "DepthBuffer");
    GLuint spotlightLightLocation = glGetUniformBlockIndex(spotlightProgramObject, "light");
    GLuint spotInverseProjectionLocation = glGetUniformLocation(spotlightProgramObject, "InverseProjection");
    glProgramUniform1i(spotlightProgramObject, spotlightColorLocation, 0);
    glProgramUniform1i(spotlightProgramObject, spotlightNormalLocation, 1);
    glProgramUniform1i(spotlightProgramObject, spotlightDepthLocation, 2);

    // Try to load and compile gamma shaders
    GLuint fragGammalightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "gamma.frag");
    GLuint gammaProgramObject = glCreateProgram();
    glAttachShader(gammaProgramObject, vertBlitShaderId);
    glAttachShader(gammaProgramObject, fragGammalightShaderId);
    glLinkProgram(gammaProgramObject);
    if (check_link_error(gammaProgramObject) < 0)
        exit(1);
    GLuint gammaTextureLocation = glGetUniformLocation(gammaProgramObject, "Texture");
    glProgramUniform1i(gammaProgramObject, gammaTextureLocation, 0);
    GLuint gammaGammaLocation = glGetUniformLocation(gammaProgramObject, "Gamma");

    // Try to load and compile freichen shaders
    //GLuint fragfreichenlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "freichen.frag");
    GLuint fragfreichenlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "sobel.frag");
    GLuint freichenProgramObject = glCreateProgram();
    glAttachShader(freichenProgramObject, vertBlitShaderId);
    glAttachShader(freichenProgramObject, fragfreichenlightShaderId);
    glLinkProgram(freichenProgramObject);
    if (check_link_error(freichenProgramObject) < 0)
        exit(1);
    GLuint freichenTextureLocation = glGetUniformLocation(freichenProgramObject, "Texture");
    glProgramUniform1i(freichenProgramObject, freichenTextureLocation, 0);
    GLuint freichenFactorLocation = glGetUniformLocation(freichenProgramObject, "Factor");

    // Try to load and compile blur shaders
    GLuint fragblurlightShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "blur.frag");
    GLuint blurProgramObject = glCreateProgram();
    glAttachShader(blurProgramObject, vertBlitShaderId);
    glAttachShader(blurProgramObject, fragblurlightShaderId);
    glLinkProgram(blurProgramObject);
    if (check_link_error(blurProgramObject) < 0)
        exit(1);
    GLuint blurTextureLocation = glGetUniformLocation(blurProgramObject, "Texture");
    glProgramUniform1i(blurProgramObject, blurTextureLocation, 0);
    GLuint blurSampleCountLocation = glGetUniformLocation(blurProgramObject, "SampleCount");
    GLuint blurDirectionLocation = glGetUniformLocation(blurProgramObject, "Direction");


   if (!checkError("Shaders"))
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
    GLuint ubo[1];
    glGenBuffers(1, ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
    GLint uboSize = 0;
    glGetActiveUniformBlockiv(pointlightProgramObject, pointlightLightLocation, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);
    glGetActiveUniformBlockiv(directionallightProgramObject, pointlightLightLocation, GL_UNIFORM_BLOCK_DATA_SIZE, &uboSize);

    // Ignore ubo size, allocate it sufficiently big for all light data structures
    uboSize = 512;

    glBufferData(GL_UNIFORM_BUFFER, uboSize, 0, GL_DYNAMIC_DRAW);
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

    // Create Fx Framebuffer Object
    GLuint fxFbo;
    GLuint fxDrawBuffers[1];
    glGenFramebuffers(1, &fxFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
    fxDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, fxDrawBuffers);

    // Create Fx textures
    GLuint fxTextures[3];
    glGenTextures(3, fxTextures);
    glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, fxTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach first fx texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkError("Framebuffers");

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

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, (int) instanceCount);
        //glDrawElements(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        glProgramUniform1f(gbufferProgramObject, timeLocation, 0.f);
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
        // Attach first fx texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);
        // Only the color buffer is used
        glClear(GL_COLOR_BUFFER_BIT);

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

        // Bind the same VAO for all lights
        glBindVertexArray(vao[2]);

        // Render point lights
        glUseProgram(pointlightProgramObject);
        struct PointLight
        {
            glm::vec3 position;
            int padding;
            glm::vec3 color;
            float intensity;
        };
        for (int i = 0; i < pointLightCount; ++i)
        {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
            PointLight p = { 
                glm::vec3( worldToView * glm::vec4((pointLightCount*cosf(t)) * sinf(t*i), 1.0, fabsf(pointLightCount*sinf(t)) * cosf(t*i), 1.0)), 0,
                glm::vec3(fabsf(cos(t+i*2.f)), 1.-fabsf(sinf(t+i)) , 0.5f + 0.5f-fabsf(cosf(t+i)) ),
                0.5f + fabsf(cosf(t+i))
            };
            PointLight * pointLightBuffer = (PointLight *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, uboSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            *pointLightBuffer = p;
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            glBindBufferBase(GL_UNIFORM_BUFFER, pointlightLightLocation, ubo[0]);
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }

        // Render directional lights
        glUseProgram(directionallightProgramObject);
        struct DirectionalLight
        {
            glm::vec3 direction;
            int padding;
            glm::vec3 color;
            float intensity;
        };
        for (int i = 0; i < directionalLightCount; ++i)
        {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
             DirectionalLight d = { 
                glm::vec3( worldToView * glm::vec4(-1.0, -1.0, 0.0, 0.0)), 0,
                glm::vec3(1.0, 1.0, 1.0),
                0.3f
            };
            DirectionalLight * directionalLightBuffer = (DirectionalLight *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, uboSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            *directionalLightBuffer = d;
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            glBindBufferBase(GL_UNIFORM_BUFFER, directionallightLightLocation, ubo[0]);
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }

        // Render spot lights
        glUseProgram(spotlightProgramObject);
        struct SpotLight
        {
            glm::vec3 position;
            float angle;
            glm::vec3 direction;
            float penumbraAngle;
            glm::vec3 color;
            float intensity;
        };
        for (int i = 0; i < spotLightCount; ++i)
        {
            glBindBuffer(GL_UNIFORM_BUFFER, ubo[0]);
            SpotLight s = { 
                glm::vec3( worldToView * glm::vec4((spotLightCount*sinf(t)) * cosf(t*i), 1.f + sinf(t * i), fabsf(spotLightCount*cosf(t)) * sinf(t*i), 1.0)), 45.f + 20.f * cos(t + i),
                glm::vec3( worldToView * glm::vec4(sinf(t*10.0+i), -1.0, 0.0, 0.0)), 60.f + 20.f * cos(t + i),
                glm::vec3(fabsf(cos(t+i*2.f)), 1.-fabsf(sinf(t+i)) , 0.5f + 0.5f-fabsf(cosf(t+i))), 1.0
            };
            SpotLight * spotLightBuffer = (SpotLight *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, uboSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            *spotLightBuffer = s;
            glUnmapBuffer(GL_UNIFORM_BUFFER);
            glBindBufferBase(GL_UNIFORM_BUFFER, spotlightLightLocation, ubo[0]);
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        }        

        // End additive blending
        glDisable(GL_BLEND);

        // Attach fx texture #1 to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        // Only the color buffer is used
        glClear(GL_COLOR_BUFFER_BIT);

        // freichen
        glUseProgram(freichenProgramObject);
        glProgramUniform1f(freichenProgramObject, freichenFactorLocation, factor);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Attach fx texture #0 to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);
        // Only the color buffer is used
        glClear(GL_COLOR_BUFFER_BIT);

        // vertical blur
        glUseProgram(blurProgramObject);
        glProgramUniform1i(blurProgramObject, blurSampleCountLocation, (int) sampleCount);
        glProgramUniform2i(blurProgramObject, blurDirectionLocation, 0, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Attach fx texture #1 to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        // Only the color buffer is used
        glClear(GL_COLOR_BUFFER_BIT);

        // horizontal blur
        glProgramUniform2i(blurProgramObject, blurDirectionLocation, 1, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Write to back buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Gamma
        glUseProgram(gammaProgramObject);
        glProgramUniform1f(gammaProgramObject, gammaGammaLocation, gamma);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Bind blit shader
        glUseProgram(blitProgramObject);
        // Upload uniforms
        // use only unit 0
        glActiveTexture(GL_TEXTURE0);
        // Bind VAO
        glBindVertexArray(vao[2]);

        // Viewport 
        glViewport( 0, 0, width/3, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        // Draw quad
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport 
        glViewport( width/3, 0, width/3, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        // Draw quad
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport 
        glViewport( width/3 * 2, 0, width/3, height/4  );
        // Bind texture
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
        // Draw quad
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

#if 1
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
        int logScroll = 0;
        char lineBuffer[512];
        imguiBeginScrollArea("aogl", width - 210, height - 310, 200, 300, &logScroll);
        sprintf(lineBuffer, "FPS %f", fps);
        imguiLabel(lineBuffer);
        imguiSlider("Speed", &speed, 0.01, 1.0, 0.01);
        imguiSlider("Point Lights", &pointLightCount, 0.0, 100.0, 1);
        imguiSlider("Directional Lights", &directionalLightCount, 0.0, 100.0, 1);
        imguiSlider("Spot Lights", &spotLightCount, 0.0, 100.0, 1);
        imguiSlider("Gamma", &gamma, 0.0, 3.0, 0.01);
        imguiSlider("Factor", &factor, 0.0, 5.0, 0.1);
        imguiSlider("Blur Samples", &sampleCount, 3.0, 65.0, 2.0);

        imguiEndScrollArea();
        imguiEndFrame();
        imguiRenderGLDraw(width, height);

        glDisable(GL_BLEND);
#endif
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
}
