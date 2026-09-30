#include "GPUProcessor.h"
#include "Utilities.h"
#include <cuda_runtime.h>
#include <iostream>

// Sobel Filters using constant memory
__constant__ int d_sobel_gx[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
__constant__ int d_sobel_gy[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};

// Macro for CUDA error checking
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while(0)


// Constructor
GPUProcessor::GPUProcessor()
    : d_input_(nullptr),
      d_output_(nullptr),
      buffer_size_(0),
      width_(0),
      height_(0)
{
    Utils::log("GPUProcessor initialized");
}

// Destructor
GPUProcessor::~GPUProcessor() {
    freeGPUMemory();
}

// Allocate GPU memory
void GPUProcessor::allocateGPUMemory(int width, int height) {
    // Calculate required buffer size
    size_t new_size = width * height * sizeof(uint8_t);

    if (new_size != buffer_size_) {
        // Free old buffers if size change
        freeGPUMemory();

        // Allocate GPU memory
        CUDA_CHECK(cudaMalloc((void**)&d_input_, new_size));
        CUDA_CHECK(cudaMalloc((void**)&d_output_, new_size));

        buffer_size_ = new_size;
        width_ = width;
        height_ = height;

        Utils::log("Allocated GPU memory: " + std::to_string(new_size) + " bytes");

    }
}

// Free GPU memory
void GPUProcessor::freeGPUMemory() {
    if (d_input_) {
        CUDA_CHECK(cudaFree(d_input_));
        d_input_ = nullptr;
    }

    if (d_output_) {
        CUDA_CHECK(cudaFree(d_output_));
        d_output_ = nullptr;
    }

    buffer_size_ = 0;
    width_ = 0;
    height_ = 0;
}

// Process single frame
cv::Mat GPUProcessor::processSingleFrame(const cv::Mat& input,
                                         double& kernel_time_ms,
                                         double& transfer_time_ms) 
{   
    // Allocate GPU memory
    allocateGPUMemory(input.cols, input.rows);
    
    // Create output Mat
    cv::Mat h_output(input.rows, input.cols, CV_8UC1); // Grayscale output
    
    // Transfer Host -> Device
    auto h2d_start = std::chrono::high_resolution_clock::now();
    CUDA_CHECK(cudaMemcpy(d_input_, input.data, buffer_size_, cudaMemcpyHostToDevice));
    double host_to_device_transfer_time = Utils::getElapsedMs(h2d_start);

    // Launch kernel
    dim3 blockSize(16, 16); // 16 x 16 threads per block
    dim3 gridSize((width_ + blockSize.x - 1) / blockSize.x,
                  (height_ + blockSize.y - 1) / blockSize.y);

    auto kernel_start = std::chrono::high_resolution_clock::now();
    
    sobelEdgeDetectionKernel<<<gridSize, blockSize>>>(d_input_, d_output_, width_, height_);
    CUDA_CHECK(cudaDeviceSynchronize()); // Wait for kernel  
    kernel_time_ms = Utils::getElapsedMs(kernel_start);

    // Transfer Device -> Host
    auto d2h_start = std::chrono::high_resolution_clock::now();
    CUDA_CHECK(cudaMemcpy(h_output.data, d_output_, buffer_size_, cudaMemcpyDeviceToHost));
    double device_to_host_transfer_time = Utils::getElapsedMs(d2h_start);

    transfer_time_ms = host_to_device_transfer_time + device_to_host_transfer_time;

    return h_output;

}

// CUDA Kernel: Sobel Edge Detection
__global__ void sobelEdgeDetectionKernel(const uint8_t* input,
                                         uint8_t* output,
                                         int width,
                                         int height)
{
    // Calculate global thread's (x, y) position
    int x = (blockDim.x * blockIdx.x) + threadIdx.x;
    int y = (blockDim.y * blockIdx.y) + threadIdx.y;

    // Check bounds (leave 1-pixel border)
    if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) {
        if (x < width && y < height) {
            output[y * width + x] = 0; // Border pixels = 0
        }
        return;
    }

    // Calculate Sobel gradients: Gx and Gy
    int gx = 0;
    int gy = 0;

    // Accessing neighboring pixels
    for (int i = 0; i < 9; i++) {
        int r = i / 3 - 1; // -1, 0, 1
        int c = i % 3 - 1; // -1, 0, 1
        int pixel = input[(y + r) * width + (x + c)];
        gx += pixel * d_sobel_gx[i];
        gy += pixel * d_sobel_gy[i];
    }

    // Gradient magnitude
    int magnitude = sqrt((float)(gx * gx + gy * gy));

    // Clamp to 0-255
    output[y * width + x] = min(255, max(0, magnitude));
}
