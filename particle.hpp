#ifndef PARTICLE_H
#define PARTICLE_H

// GLEW must come first
#include <GL/glew.h>

// Then other OpenGL/window headers
#include <GL/glx.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// OpenCL headers
#include <CL/cl.h>
#include <CL/cl_gl.h> // This includes OpenCL-OpenGL interop definitions

// Add after other OpenGL headers
#include <CL/cl_egl.h>
#include <EGL/egl.h>

// Standard headers
#include <array>
#include <cstring>
#include <exception>
#include <iostream>
#include <math.h>
#include <string>
#include <time.h>
#include <vector>

// Add at the top with other includes
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX

#define MAX_SOURCE_SIZE 10000
#define RED "\e[1;38;2;225;20;20m"
#define WHITE "\e[1;38;2;255;251;214m"
#define YELLO "\e[1;38;2;255;200;0m"
#define ORANGE "\e[1;38;2;255;120;10m"
#define GREEN "\e[1;38;2;0;175;117m"

#define PI 3.14159265358979323846
#define FAR 50
#define NEAR 0.1

extern const unsigned int W;
extern const unsigned int H;
extern int N;
typedef unsigned int t_uint;

extern GLFWwindow *window;
extern float hsv[3];

// Ensure proper alignment and packing for OpenCL-OpenGL interop
struct alignas(32) Particle
{
    alignas(16) float pos[4]; // xyz + padding for alignment
    alignas(16) float vel[4]; // xyz + padding for alignment
};

// Mass for the mouse
struct Mass
{
    float x{0}, y{0}, z{0};    // position
    int n{0};                  // number of particles
    std::array<float, 10> m{}; // masses
    float att{0.05f};          // attraction
    int nPart{0};              // number of particles
};

// Buffers for the particles
struct Buffers
{
    GLuint mat;     // the texture
    GLuint vao;     // vertex array object
    GLuint vm;      // vertex matrix
    GLuint vbo;     // vertex buffer object
    GLuint shaders; // shaders
    GLuint mx;      // mouse x
    GLuint my;      // mouse y
    GLuint hsv;     // hue, saturation, value
    float bl{0.0f}; // brightness
    float pt{1.0f}; // point size

    std::array<float, 16> camx{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; // camera x

    std::array<float, 16> camz{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; // camera z

    std::array<float, 16> trans{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -1.5f, 1}; // transformation

    std::array<float, 16> p{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -FAR / (FAR - NEAR), -1, 0, 0, -FAR *NEAR / (FAR - NEAR),
                            0}; // projection
};

extern Buffers g_bufs;
extern Mass mouse;
extern bool freezehue;
extern bool go;
extern bool explode;
extern bool newParticles;

extern size_t global_item_size;
extern size_t local_item_size;

extern cl_program program;
extern cl_kernel ker_init;
extern cl_kernel ker_acc;
extern cl_kernel ker_move;
extern cl_kernel ker_gen;
extern cl_kernel ker_zoomin;
extern cl_kernel ker_zoomout;
extern cl_command_queue command_queue;
extern cl_device_id device_id;

extern bool circle;

// Add these external declarations
extern cl_int ret;
extern cl_mem memobj;
extern cl_context context;

void getcontext();
void glinit();
void glend();
void loop();
void keyholds(GLFWwindow *window);
std::string filetostr(const std::string &filename);
void getcontext();
void clinit();
void clReset();
void clend();

// Add type alias for backward compatibility
using t_bufs = Buffers;
using t_mass = Mass;
using t_p = Particle;

#endif
