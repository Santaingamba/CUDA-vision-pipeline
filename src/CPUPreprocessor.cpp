#include "CPUPreprocessor.h"
#include "Utilities.h"
#include <opencv2/opencv.hpp>


    
// Constructor
CPUPreprocessor::CPUPreprocessor(size_t num_threads)
    : num_threads_(num_threads) 
{
    Utils::log("CPUPreprocessor initialized with " + std::to_string(num_threads_) + " threads");
}

// Destructor
CPUPreprocessor::~CPUPreprocessor(){}

// Process a single frame (grayscale conversion)
cv::Mat CPUPreprocessor::processSingleFrame(const cv::Mat& input){
    cv::Mat processed;
    processed = convertToGrayScale(input);

    return processed;
}

// Process multiple frames with threading
void CPUPreprocessor::processFrameBatch(std::vector<cv::Mat>& frames){
    if (num_threads_ == 1) {
        // Single-threaded path
        for (auto& frame: frames) {
            frame = processSingleFrame(frame);
        }
    } else {
        // Multi-threaded path
        size_t frames_per_thread = (frames.size() + num_threads_ - 1) / num_threads_;
        thread_pool_.clear();
        thread_pool_.reserve(num_threads_);
        
        for (size_t i = 0; i < num_threads_; i++) {
            size_t start = i * frames_per_thread;
            size_t end = std::min(start + frames_per_thread, frames.size());
            
            // Create thread with lambda
            thread_pool_.emplace_back([this, &frames, start, end]() {
                this->processFrameRange(frames, start, end);
            });
        }

        // Wait for all threads to complete
        for (auto& thread: thread_pool_) {
            thread.join();
        }
        
    }
}

// Helper: Convert to grayscale
cv::Mat CPUPreprocessor::convertToGrayScale(const cv::Mat& input){
    cv::Mat gray;
    if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    return gray;
}

// Helper: Normalize pixels (0-255 -> 0.0-1.0)
cv::Mat CPUPreprocessor::normalizePixels(const cv::Mat& input){
    cv::Mat normalized;
    input.convertTo(normalized, CV_32F, 1.0 / 255.0);
    return normalized;
}

// Worker function for thread pool
void CPUPreprocessor::processFrameRange(std::vector<cv::Mat>& frames, size_t start, size_t end){
    for (size_t i = start; i < end && i < frames.size(); i++) {
        frames[i] = processSingleFrame(frames[i]);
    }
}