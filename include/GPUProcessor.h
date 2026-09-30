#ifndef GPU_PROCESSOR_H
#define GPU_PROCESSOR_H

#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>

class GPUProcessor {
    private:
        // GPU memory pointers
        uint8_t* d_input_;      // Device input buffer
        uint8_t* d_output_;     // Device output buffer
        size_t buffer_size_;    // Current buffer size
        int width_;             // Frame width
        int height_;            // Frame height

    public:
        // Constructor
        GPUProcessor();

        // Destructor - cleanup GPU memory
        ~GPUProcessor();

        // Process single frame with Sobel edge detection
        cv::Mat processSingleFrame(const cv::Mat& input,
                                   double& kernel_time_ms,
                                   double& transfer_time_ms);

        // Process batch of frames
        void processFrameBatch(std::vector<cv::Mat>& frames,
                               double& total_kernel_time_ms,
                               double& total_transfer_time_ms);
        
    private:
        // Allocate GPU memory for frame size
        void allocateGPUMemory(int width, int height);

        // Free GPU memory
        void freeGPUMemory();

        // Check CUDA errors
        void checkCudaError(cudaError_t error, const char* file, int line);

};

// CUDA kernel declaration (implemented in .cu file)
__global__ void sobelEdgeDetectionKernel(const uint8_t* input,
                                         uint8_t* output,
                                         int width,
                                         int height);

#endif // GPU_PROCESSOR_H                                         
