#include "particle.hpp"
#include <GLFW/glfw3.h>

float hsv[3] = {0, .6, 1.0};
Buffers g_bufs;
Mass mouse;

using namespace std;

// Add these forward declarations after the includes
void cursor(GLFWwindow *window, double x, double y);
void button(GLFWwindow *window, int button, int action, int mods);
void scroll(GLFWwindow *window, double x, double y);
void keys(GLFWwindow *window, int key, int scan, int action, int mods);

void getshader(Buffers *g_bufs)
{
    t_uint vertexshader;
    t_uint fragmentshader;
    // Get shader source as strings
    std::string vsrc_str = filetostr("particle.vs");
    std::string fsrc_str = filetostr("particle.fs");
    // Get C-style string pointers
    const GLchar *vsrc = vsrc_str.c_str();
    const GLchar *fsrc = fsrc_str.c_str();

    vertexshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexshader, 1, &vsrc, NULL);
    glCompileShader(vertexshader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexshader, 512, NULL, infoLog);
        cout << RED << "Vertex shader compilation failed:\n" << infoLog << endl;
    }

    fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentshader, 1, &fsrc, NULL);
    glCompileShader(fragmentshader);

    glGetShaderiv(fragmentshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentshader, 512, NULL, infoLog);
        cout << RED << "Fragment shader compilation failed:\n" << infoLog << endl;
    }

    g_bufs->shaders = glCreateProgram();
    glAttachShader(g_bufs->shaders, vertexshader);
    glAttachShader(g_bufs->shaders, fragmentshader);
    glLinkProgram(g_bufs->shaders);

    glGetProgramiv(g_bufs->shaders, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(g_bufs->shaders, 512, NULL, infoLog);
        cout << RED << "Shader program linking failed:\n" << infoLog << endl;
    }

    glDeleteShader(vertexshader);
    glDeleteShader(fragmentshader);

    // Use glGetUniformLocation for uniforms
    g_bufs->mat = glGetUniformLocation(g_bufs->shaders, "p");
    g_bufs->mx = glGetUniformLocation(g_bufs->shaders, "mx");
    g_bufs->my = glGetUniformLocation(g_bufs->shaders, "my");
    g_bufs->hsv = glGetUniformLocation(g_bufs->shaders, "hsv");

    // Fix pointer access with ->
    glGenBuffers(1, &g_bufs->vbo);
    glGenVertexArrays(1, &g_bufs->vao);
    glBindVertexArray(g_bufs->vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_bufs->vbo);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(Particle), NULL, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), NULL);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    // Forward declare these functions at the top of the file
    glfwSetCursorPosCallback(window, cursor);
    glfwSetMouseButtonCallback(window, button);
    glfwSetScrollCallback(window, scroll);
    glfwSetKeyCallback(window, keys);

    // Fix pointer access with ->
    g_bufs->bl = 0.0f;
    g_bufs->pt = 1;
    g_bufs->camx[0] = 1;
    g_bufs->camx[15] = 1;
    g_bufs->camx[10] = 1;
    g_bufs->camx[15] = 1;
    g_bufs->p[0] = 1.0 / tan(90 / 2 * PI / 180);
    g_bufs->p[5] = g_bufs->p[0];
    glPointSize(g_bufs->pt);
    glUseProgram(g_bufs->shaders);
}

void cursor(GLFWwindow *window, double x, double y)
{
    mouse.x = (float)x * 2 / W - 1.0;
    mouse.y = -((float)y * 2 / H - 1.0);
}

void button(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mouse.n < 5)
    {
        mouse.m[2 * mouse.n] = mouse.x;
        mouse.m[2 * mouse.n + 1] = mouse.y;
        mouse.n++;
    }
}

void scroll(GLFWwindow *window, double x, double y)
{
    if (x > 0)
        mouse.att += 0.001;
    if (x < 0)
        mouse.att -= (mouse.att >= 0.002 ? 0.001 : 0);

    if (y != 0)
    {
        // Acquire GL objects before using them
        ret = clEnqueueAcquireGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to acquire GL objects in scroll: " << ret << endl;
            exit(1);
        }

        if (y > 0)
        {
            ret = clEnqueueNDRangeKernel(command_queue, ker_zoomout, 1, NULL, &global_item_size, &local_item_size, 0,
                                         NULL, NULL);
            for (int i = 0; i < mouse.n; i++)
            {
                mouse.m[2 * i] *= 0.9;
                mouse.m[2 * i + 1] *= 0.9;
            }
        }
        if (y < 0)
        {
            ret = clEnqueueNDRangeKernel(command_queue, ker_zoomin, 1, NULL, &global_item_size, &local_item_size, 0,
                                         NULL, NULL);
            for (int i = 0; i < mouse.n; i++)
            {
                mouse.m[2 * i] /= 0.9;
                mouse.m[2 * i + 1] /= 0.9;
            }
        }

        // Release GL objects after using them
        ret = clEnqueueReleaseGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to release GL objects in scroll: " << ret << endl;
            exit(1);
        }

        clFinish(command_queue);
        loop();
    }
}

void keys(GLFWwindow *window, int key, int scan, int action, int mods)
{
    (void)scan;
    (void)mods;
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        go = !go;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        explode = !explode;
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        mouse.n = 0;
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        freezehue = !freezehue;
    if (key == GLFW_KEY_N && action == GLFW_PRESS)
        newParticles = !newParticles;
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
        clReset();
}

void keyholds(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        g_bufs.bl = (g_bufs.bl + 0.01 > 1 ? 1 : g_bufs.bl + 0.01);
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        g_bufs.bl = (g_bufs.bl - 0.01 < 0 ? 0 : g_bufs.bl - 0.01);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        hsv[1] = (hsv[1] + 0.01 > 1 ? 1 : hsv[1] + 0.01);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        hsv[1] = (hsv[1] - 0.01 < 0 ? 0 : hsv[1] - 0.01);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        hsv[2] = (hsv[2] + 0.01 > 1 ? 1 : hsv[2] + 0.01);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        hsv[2] = (hsv[2] - 0.01 < 0 ? 0 : hsv[2] - 0.01);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_bufs.trans[14] += 0.02;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_bufs.trans[14] -= 0.02;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        mouse.z += 0.02;
        g_bufs.trans[12] += 0.02;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        mouse.z -= 0.02;
        g_bufs.trans[12] -= 0.02;
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
    {
        g_bufs.pt += glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ? 0.02 : -0.02;
        glPointSize(g_bufs.pt);
    }
}

void glinit()
{
    mouse.x = 0;
    mouse.y = 0;
    mouse.z = 0;
    mouse.n = 0;
    mouse.att = 0.05;

    if (!glfwInit())
    {
        cout << RED << "Failed to initialize GLFW" << endl;
        exit(1);
    }

    // Force GLFW to use NVIDIA GPU
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE); // Make sure window is visible

    window = glfwCreateWindow(W, H, "FPS", NULL, NULL);
    if (!window)
    {
        cout << RED << "Failed to create GLFW window" << endl;
        glfwTerminate();
        exit(1);
    }

    // Make context current before any GL calls
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        cout << RED << "Failed to initialize GLEW: " << glewGetErrorString(err) << endl;
        exit(1);
    }

    // Clear any GL error that might have been set by GLEW init
    glGetError();

    // Print GPU info
    cout << YELLO << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << YELLO << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    cout << YELLO << "OpenGL Vendor: " << glGetString(GL_VENDOR) << endl;
    cout << YELLO << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;

    // Check if we're actually using NVIDIA
    std::string vendor(reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
    if (vendor.find("NVIDIA") == std::string::npos)
    {
        cout << RED << "Not using NVIDIA GPU for OpenGL. Please ensure NVIDIA is set as primary GPU." << endl;
        cout << RED << "You might need to:" << endl;
        cout << RED << "1. Set NVIDIA as primary GPU in BIOS" << endl;
        cout << RED << "2. Use nvidia-prime or similar to set NVIDIA as primary" << endl;
        cout << RED << "3. Set DRI_PRIME=1 environment variable" << endl;
        exit(1);
    }

    // Create and bind VAO first
    glGenVertexArrays(1, &g_bufs.vao);
    glBindVertexArray(g_bufs.vao);

    // Create and bind VBO
    glGenBuffers(1, &g_bufs.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_bufs.vbo);

    // Initialize buffer with zeros
    const size_t buffer_size = N * sizeof(Particle);
    std::vector<float> zeros(buffer_size / sizeof(float), 0.0f);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, zeros.data(), GL_DYNAMIC_DRAW);

    // Set up vertex attributes for position only
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)0);
    glEnableVertexAttribArray(0);

    // Keep VAO bound but unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Ensure GL operations are complete
    glFinish();
    glFlush();

    // Now initialize OpenCL
    getcontext();
    getshader(&g_bufs);

    // Set up other GL state
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glfwSetCursorPosCallback(window, cursor);
    glfwSetMouseButtonCallback(window, button);
    glfwSetScrollCallback(window, scroll);
    glfwSetKeyCallback(window, keys);

    // Fix pointer access with ->
    g_bufs.bl = 0.0f; // set the blur to 0
    g_bufs.pt = 1;    // set the point size to 1
    g_bufs.camx[0] = 1;
    g_bufs.camx[15] = 1;
    g_bufs.camx[10] = 1;
    g_bufs.camx[15] = 1;
    g_bufs.p[0] = 1.0 / tan(90 / 2 * PI / 180); // set the projection to 90 degrees
    g_bufs.p[5] = g_bufs.p[0];
    glPointSize(g_bufs.pt);
    glUseProgram(g_bufs.shaders);
}

void glend()
{
    glDeleteVertexArrays(1, &g_bufs.vao);
    glDeleteBuffers(1, &g_bufs.vbo);
    glDeleteProgram(g_bufs.shaders);
    glfwDestroyWindow(window);
    glfwTerminate();
}
