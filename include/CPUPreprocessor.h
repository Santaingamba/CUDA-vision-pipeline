#ifndef CPU_PREPROCESSOR_H
#define CPU_PREPROCESSOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <thread>

class CPUPreprocessor {
    private:
        size_t num_threads_;                      // Number of threads in pool
        std::vector<std::thread> thread_pool_;    // Thread pool

    public:
        // Constructor
        CPUPreprocessor(size_t num_threads = 1);

        // Destructor
        ~CPUPreprocessor();

        // Process a single frame (grayscale conversion)
        cv::Mat processSingleFrame(const cv::Mat& input);

        // Process multiple frames with threading
        void processFrameBatch(std::vector<cv::Mat>& frames);

    private:
        // Helper: Convert to grayscale
        static cv::Mat convertToGrayScale(const cv::Mat& input);

        // Helper: Normalize pixels (0-255 -> 0.0-1.0)
        static cv::Mat normalizePixels(const cv::Mat& input);

        // Worker function for thread pool
        void processFrameRange(std::vector<cv::Mat>& frame, size_t start, size_t end);
};

#endif // CPU_PREPROCESSOR_H