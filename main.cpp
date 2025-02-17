#include "particle.hpp"
#include <cstring>
#include <execinfo.h> // for backtrace
#include <fstream>
#include <signal.h>
#include <sstream>
using namespace std;

GLFWwindow *window;

const unsigned int W = 1400; // window width
const unsigned int H = 1400; // window height
int N = 1000;                // number of particles

int db;                // debug
bool freezehue = 0;    // freeze hue
bool go = 1;           // if the particles are moving
bool explode = 0;      // if the particles are exploding
bool newParticles = 0; // if new particles are being created
bool circle = 0;       // if the particles are in a circle

int nbFrames = 0; // number of frames
double lastTime;  // last time to update FPS

// Convert file to string
std::string filetostr(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Multiply two matrices
void mult(const float *a, const float *b, float *result)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++)
            {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

// Overload for std::array
void mult(const std::array<float, 16> &a, const std::array<float, 16> &b, std::array<float, 16> &result)
{
    mult(a.data(), b.data(), result.data());
}

// Get the matrix from the mouse
void getmatrix(float *mat)
{
    float t[16];

    g_bufs.camx[5] = cos(mouse.y * PI);
    g_bufs.camx[6] = sin(mouse.y * PI);
    g_bufs.camx[9] = -sin(mouse.y * PI);
    g_bufs.camx[10] = cos(mouse.y * PI);
    g_bufs.camz[0] = cos(mouse.x * PI);
    g_bufs.camz[2] = sin(mouse.x * PI);
    g_bufs.camz[8] = -sin(mouse.x * PI);
    g_bufs.camz[10] = cos(mouse.x * PI);

    // Use data() to get pointer to array contents
    mult(g_bufs.camz.data(), g_bufs.camx.data(), t);
    memcpy(mat, t, 16 * sizeof(float));
}

void loop()
{
    static int nPart = 0;               // number of particles
    char buf[20];                       // buffer for FPS
    double currentTime = glfwGetTime(); // current time
    nbFrames++;                         // number of frames
    if (currentTime - lastTime >= 1.0)  // update FPS every second
    {
        sprintf(buf, "%d FPS\n", nbFrames);
        glfwSetWindowTitle(window, buf);
        nbFrames = 0;
        lastTime += 1.0;
    }
    keyholds(window);
    if (!freezehue)
        hsv[0] += 0.001;
    if (hsv[0] > 1)
        hsv[0] -= 1;
    if (go)
    {
        // Ensure GL is done
        glFinish();
        glFlush();

        // Acquire GL objects
        ret = clEnqueueAcquireGLObjects(command_queue, 1, &memobj, 0, nullptr, nullptr);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to acquire GL objects: " << ret << endl;
            exit(1);
        }

        // Execute kernels
        if (newParticles)
        {
            clSetKernelArg(ker_gen, 1, sizeof(Mass), &mouse);
            ret = clEnqueueNDRangeKernel(command_queue, ker_gen, 1, nullptr, &global_item_size, nullptr, 0, nullptr,
                                         nullptr);
        }

        if (!explode)
        {
            clSetKernelArg(ker_acc, 1, sizeof(Mass), &mouse);
            ret = clEnqueueNDRangeKernel(command_queue, ker_acc, 1, nullptr, &global_item_size, nullptr, 0, nullptr,
                                         nullptr);
        }

        ret = clEnqueueNDRangeKernel(command_queue, ker_move, 1, nullptr, &global_item_size, nullptr, 0, nullptr,
                                     nullptr);

        // Ensure CL is done
        clFinish(command_queue);

        // Release GL objects
        ret = clEnqueueReleaseGLObjects(command_queue, 1, &memobj, 0, nullptr, nullptr);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to release GL objects: " << ret << endl;
            exit(1);
        }

        // Final sync
        clFinish(command_queue);
    }
    float tmp[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; // identity matrix
    if (!go)
        getmatrix(tmp);
    float tmp2[16];

    // Use data() to get pointers for array contents
    mult(tmp, g_bufs.trans.data(), tmp2); // multiply the transformation matrix with the identity matrix
    mult(tmp2, g_bufs.p.data(), tmp);     // multiply the projection matrix with the transformation matrix

    glUniformMatrix4fv(g_bufs.mat, 1, GL_FALSE, &tmp[0]); // set the matrix for the shader
    glUniform1f(g_bufs.mx, mouse.x);                      // set the mouse x for the shader
    glUniform1f(g_bufs.my, mouse.y);                      // set the mouse y for the shader
    glUniform3f(g_bufs.hsv, hsv[0], hsv[1], hsv[2]);      // set the hue, saturation, value for the shader

    glClearColor(g_bufs.bl, g_bufs.bl, g_bufs.bl, 1.0f); // set the clear color for the shader
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear the screen
    glBindVertexArray(g_bufs.vao);                       // bind the vertex array
    glDrawArrays(GL_TRIANGLES, 0, N);                    // draw the particles
    glBindVertexArray(g_bufs.vao);                       // bind the vertex array
    glfwSwapBuffers(window);                             // swap the buffers
}

void signal_handler(int signum)
{
    cout << RED << "Caught signal " << signum << endl;
    if (signum == SIGSEGV)
    {
        cout << RED << "Segmentation fault detected!" << endl;
        // Print backtrace if possible
        void *array[10];
        size_t size = backtrace(array, 10);
        char **strings = backtrace_symbols(array, size);
        cout << RED << "Backtrace:" << endl;
        for (size_t i = 0; i < size; i++)
        {
            cout << RED << strings[i] << endl;
        }
        free(strings);
    }
    exit(1);
}

int main(int ac, char **av)
{
    // Register signal handler
    signal(SIGSEGV, signal_handler);

    lastTime = glfwGetTime();
    if (ac == 3 && !strcmp(av[2], "-s"))
        circle = 1;
    if (ac >= 2)
        N = atoi(av[1]);
    if (N < 250 || N > 5000000 || ac > 3 || ac == 1)
    {
        printf(ORANGE);
        printf("Usage: ./particle_system number of particles [-s]\n");
        printf("\t\t250 <= number of particles <= 5000000\n");
        exit(1);
    }

    // initialize the random number generator
    srand(time(NULL));

    // Initialize OpenGL first and ensure it's successful
    glinit();

    // Ensure GL context is current
    glfwMakeContextCurrent(window);

    // Force a sync point
    glFinish();

    // Now initialize OpenCL
    try
    {
        clinit();
    }
    catch (const std::exception &e)
    {
        cout << RED << "OpenCL initialization failed: " << e.what() << endl;
        glend();
        exit(1);
    }

    while (!glfwWindowShouldClose(window))
    {
        loop();
        glfwPollEvents();
    }

    // end the OpenCL and OpenGL
    clend();
    glend();
    return (0);
}
