#include "particle.hpp"
#include <dlfcn.h>
using namespace std;

cl_int ret;            // return value
cl_uint uret;          // unsigned return value
cl_mem memobj;         // memory object
cl_kernel ker_init;    // initialize kernel
cl_kernel ker_acc;     // accelerate kernel
cl_kernel ker_move;    // move kernel
cl_kernel ker_gen;     // generate kernel
cl_kernel ker_zoomout; // zoom out kernel
cl_kernel ker_zoomin;  // zoom in kernel
cl_program program;
cl_command_queue command_queue;
cl_context context;

size_t global_item_size;
size_t local_item_size = 250;
cl_platform_id platform_id;
cl_device_id device_id;

std::string getOpenCLErrorString(cl_int error);

class OpenCLContext
{
  private:
    cl_context context{nullptr};
    cl_command_queue commandQueue{nullptr};
    cl_program program{nullptr};
    cl_mem memobj{nullptr};
    std::vector<cl_kernel> kernels;

  public:
    OpenCLContext() = default;
    ~OpenCLContext()
    {
        cleanup();
    }

    void init()
    {
        try
        {
            createContext();
            createCommandQueue();
            createProgram();
            createKernels();
        }
        catch (const std::runtime_error &e)
        {
            cleanup();
            throw;
        }
    }

  private:
    void createContext()
    {
        cl_platform_id platformId = nullptr;
        cl_device_id deviceId = nullptr;

        cl_int ret = clGetPlatformIDs(1, &platformId, nullptr);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to get platform ID");
        }

        ret = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 1, &deviceId, nullptr);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to get device ID");
        }

        Display *display = glXGetCurrentDisplay();
        GLXContext glxContext = glXGetCurrentContext();

        cl_context_properties properties[] = {CL_GL_CONTEXT_KHR,
                                              (cl_context_properties)glxContext,
                                              CL_GLX_DISPLAY_KHR,
                                              (cl_context_properties)display,
                                              CL_CONTEXT_PLATFORM,
                                              (cl_context_properties)platformId,
                                              0};

        context = clCreateContext(properties, 1, &deviceId, nullptr, nullptr, &ret);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to create OpenCL context");
        }
    }

    void createCommandQueue()
    {
        cl_device_id deviceId = nullptr;
        cl_int ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &deviceId, nullptr);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to get device ID");
        }

        cl_queue_properties properties[] = {0};
        commandQueue = clCreateCommandQueueWithProperties(context, deviceId, properties, &ret);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to create command queue");
        }
    }

    void createProgram()
    {
        std::string source = filetostr("kernel.cl");
        const char *source_str = source.c_str();
        size_t source_size = source.length();

        program = clCreateProgramWithSource(context, 1, &source_str, &source_size, &ret);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to create program");
        }

        ret = clBuildProgram(program, 1, &device_id, nullptr, nullptr, nullptr);
        if (ret != CL_SUCCESS)
        {
            size_t log_size;
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            std::vector<char> build_log(log_size + 1);
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, build_log.data(), nullptr);
            throw std::runtime_error(std::string("Program build failed: ") + build_log.data());
        }
    }

    void createKernels()
    {
        // Create all your kernels here
        cl_kernel kernel = clCreateKernel(program, "init", &ret);
        if (ret != CL_SUCCESS)
        {
            throw std::runtime_error("Failed to create kernel");
        }
        kernels.push_back(kernel);
        // Add other kernels similarly
    }

    void cleanup()
    {
        for (auto kernel : kernels)
        {
            clReleaseKernel(kernel);
        }
        if (program)
            clReleaseProgram(program);
        if (memobj)
            clReleaseMemObject(memobj);
        if (commandQueue)
            clReleaseCommandQueue(commandQueue);
        if (context)
            clReleaseContext(context);
    }
};

static void init_particles()
{
    try
    {
        // Create kernels with error checking
        ker_acc = clCreateKernel(program, "accelerate", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create accelerate kernel");

        ker_move = clCreateKernel(program, "move", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create move kernel");

        ker_gen = clCreateKernel(program, "gen", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create gen kernel");

        ker_zoomout = clCreateKernel(program, "zoomout", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create zoomout kernel");

        ker_zoomin = clCreateKernel(program, "zoomin", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create zoomin kernel");

        if (circle)
            ker_init = clCreateKernel(program, "init2", &ret);
        else
            ker_init = clCreateKernel(program, "init", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create init kernel");

        // Set kernel arguments with error checking
        ret = clSetKernelArg(ker_acc, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set accelerate kernel arg");

        ret = clSetKernelArg(ker_move, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set move kernel arg");

        ret = clSetKernelArg(ker_gen, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set gen kernel arg");

        ret = clSetKernelArg(ker_zoomout, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set zoomout kernel arg");

        ret = clSetKernelArg(ker_zoomin, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set zoomin kernel arg");

        ret = clSetKernelArg(ker_init, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set init kernel arg");

        // Print debug info
        cout << YELLO << "Memory object handle: " << memobj << endl;
        cout << YELLO << "Program handle: " << program << endl;
        cout << YELLO << "Command queue handle: " << command_queue << endl;

        // Acquire GL objects before using them
        ret = clEnqueueAcquireGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to acquire GL objects");

        // Initialize particles
        ret = clEnqueueNDRangeKernel(command_queue, ker_init, 1, NULL, &global_item_size, &local_item_size, 0, NULL,
                                     NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to enqueue init kernel");

        // Release GL objects after using them
        ret = clEnqueueReleaseGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to release GL objects");

        // Make sure everything is done
        ret = clFinish(command_queue);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to finish command queue");
    }
    catch (const std::runtime_error &e)
    {
        cout << RED << "Error in init_particles: " << e.what() << " (error code: " << ret << ")" << endl;
        exit(1);
    }
}

void clReset()
{
    ret = clEnqueueAcquireGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        cout << RED << "Failed to acquire GL objects in reset: " << ret << endl;
        exit(1);
    }

    ret = clSetKernelArg(ker_init, 0, sizeof(cl_mem), (void *)&memobj);
    ret = clEnqueueNDRangeKernel(command_queue, ker_init, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);

    ret = clEnqueueReleaseGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        cout << RED << "Failed to release GL objects in reset: " << ret << endl;
        exit(1);
    }

    clFinish(command_queue);

    mouse.z = 0;
    g_bufs.trans[12] = 0;
    g_bufs.trans[14] = -1.5;
}

void getcontext()
{
    try
    {
        // Load required libraries
        void *opencl_handle = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_GLOBAL);
        void *nvidia_handle = dlopen("libnvidia-opencl.so.1", RTLD_NOW | RTLD_GLOBAL);
        if (!opencl_handle || !nvidia_handle)
        {
            cout << RED << "Failed to load OpenCL libraries" << endl;
            exit(1);
        }

        // Get platform count
        cl_uint num_platforms = 0;
        cl_int ret = clGetPlatformIDs(0, nullptr, &num_platforms);
        if (ret != CL_SUCCESS || num_platforms == 0)
        {
            cout << RED << "Failed to get OpenCL platforms" << endl;
            exit(1);
        }

        // Get all platforms
        std::vector<cl_platform_id> platforms(num_platforms);
        ret = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to get OpenCL platforms" << endl;
            exit(1);
        }

        // Find NVIDIA platform
        bool found = false;
        for (const auto &platform : platforms)
        {
            char vendor[256];
            clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(vendor), vendor, nullptr);
            if (std::string(vendor).find("NVIDIA") != std::string::npos)
            {
                platform_id = platform;
                found = true;
                break;
            }
        }

        if (!found)
        {
            cout << RED << "NVIDIA OpenCL platform not found" << endl;
            exit(1);
        }

        // Get GPU device
        ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, nullptr);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to get GPU device" << endl;
            exit(1);
        }

        // Get OpenGL context and display
        Display *display = glXGetCurrentDisplay();
        GLXContext glxContext = glXGetCurrentContext();
        if (!display || !glxContext)
        {
            cout << RED << "No valid OpenGL context or display found" << endl;
            exit(1);
        }

        // Create OpenCL context with GL interop
        cl_context_properties properties[] = {CL_GL_CONTEXT_KHR,
                                              (cl_context_properties)glxContext,
                                              CL_GLX_DISPLAY_KHR,
                                              (cl_context_properties)display,
                                              CL_CONTEXT_PLATFORM,
                                              (cl_context_properties)platform_id,
                                              0};

        context = clCreateContext(properties, 1, &device_id, nullptr, nullptr, &ret);
        if (ret != CL_SUCCESS)
        {
            cout << RED << "Failed to create OpenCL context" << endl;
            exit(1);
        }
    }
    catch (const std::exception &e)
    {
        cout << RED << "Exception during OpenCL initialization" << endl;
        exit(1);
    }
}

void clinit()
{
    char buf[20];

    global_item_size = N;

    // Create command queue
    command_queue = clCreateCommandQueueWithProperties(context, device_id, nullptr, &ret);
    if (ret != CL_SUCCESS)
    {
        cout << RED << "Failed to create command queue: " << ret << endl;
        exit(1);
    }

    // Ensure GL is done
    glFinish();
    glFlush();

    // Create shared buffer
    memobj = clCreateFromGLBuffer(context, CL_MEM_READ_WRITE, g_bufs.vbo, &ret);
    if (ret != CL_SUCCESS)
    {
        cout << RED << "Failed to create shared buffer: " << ret << endl;
        exit(1);
    }

    // Create and build program
    std::string kernel_source = filetostr("kernel.cl");
    const char *kernel_str = kernel_source.c_str();
    size_t kernel_size = kernel_source.length();

    program = clCreateProgramWithSource(context, 1, &kernel_str, &kernel_size, &ret);
    if (ret != CL_SUCCESS)
    {
        cout << RED << "Failed to create program: " << ret << endl;
        exit(1);
    }

    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        std::vector<char> build_log(log_size + 1);
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, build_log.data(), NULL);
        cout << RED << "Build error: " << build_log.data() << endl;
        exit(1);
    }

    // Initialize particles before creating kernels
    try
    {
        // Create kernels
        ker_acc = clCreateKernel(program, "accelerate", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create accelerate kernel");

        ker_move = clCreateKernel(program, "move", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create move kernel");

        ker_gen = clCreateKernel(program, "gen", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create gen kernel");

        ker_zoomout = clCreateKernel(program, "zoomout", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create zoomout kernel");

        ker_zoomin = clCreateKernel(program, "zoomin", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create zoomin kernel");

        if (circle)
            ker_init = clCreateKernel(program, "init2", &ret);
        else
            ker_init = clCreateKernel(program, "init", &ret);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to create init kernel");

        // Set kernel arguments
        ret = clSetKernelArg(ker_acc, 0, sizeof(cl_mem), &memobj);
        ret |= clSetKernelArg(ker_move, 0, sizeof(cl_mem), &memobj);
        ret |= clSetKernelArg(ker_gen, 0, sizeof(cl_mem), &memobj);
        ret |= clSetKernelArg(ker_zoomout, 0, sizeof(cl_mem), &memobj);
        ret |= clSetKernelArg(ker_zoomin, 0, sizeof(cl_mem), &memobj);
        ret |= clSetKernelArg(ker_init, 0, sizeof(cl_mem), &memobj);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to set kernel arguments");

        // Initialize particles
        ret = clEnqueueAcquireGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to acquire GL objects");

        ret = clEnqueueNDRangeKernel(command_queue, ker_init, 1, NULL, &global_item_size, &local_item_size, 0, NULL,
                                     NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to execute init kernel");

        ret = clEnqueueReleaseGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to release GL objects");

        ret = clFinish(command_queue);
        if (ret != CL_SUCCESS)
            throw std::runtime_error("Failed to finish command queue");
    }
    catch (const std::runtime_error &e)
    {
        cout << RED << "Error in clinit: " << e.what() << " (error code: " << ret << ")" << endl;
        exit(1);
    }
}

void clend()
{
    ret = clFlush(command_queue);
    ret = clEnqueueAcquireGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
    ret = clEnqueueReleaseGLObjects(command_queue, 1, &memobj, 0, NULL, NULL);
    clFinish(command_queue);

    ret = clReleaseKernel(ker_init);
    ret = clReleaseKernel(ker_acc);
    ret = clReleaseKernel(ker_move);
    ret = clReleaseKernel(ker_gen);
    ret = clReleaseKernel(ker_zoomout);
    ret = clReleaseKernel(ker_zoomin);

    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(memobj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
}

std::string getOpenCLErrorString(cl_int error)
{
    switch (error)
    {
    case CL_SUCCESS:
        return "Success";
    case CL_DEVICE_NOT_FOUND:
        return "Device not found";
    case CL_DEVICE_NOT_AVAILABLE:
        return "Device not available";
    case CL_INVALID_PLATFORM:
        return "Invalid platform";
    case CL_INVALID_DEVICE:
        return "Invalid device";
    case CL_INVALID_CONTEXT:
        return "Invalid context";
    case CL_INVALID_GL_OBJECT:
        return "Invalid OpenGL object";
    default:
        return "Unknown error code: " + std::to_string(error);
    }
}
