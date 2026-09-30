#include "FrameLoader.h"
#include "Utilities.h"
#include <iostream>

// Constructor: open video file
FrameLoader::FrameLoader(const std::string& video_path) // lowercase string
    : video_(video_path), // Member initializer list
      current_frame_index_(0),
      total_frames_(0),
      frame_width_(0),
      frame_height_(0),
      video_fps_(0.0)
{
    if (!video_.isOpened()) {
        Utils::log("Error: Could not open video file " + video_path);
        return;
    }

    Utils::log("Video file opened successfully: " + video_path);

    total_frames_ = static_cast<int>(video_.get(cv::CAP_PROP_FRAME_COUNT));
    frame_width_ = static_cast<int>(video_.get(cv::CAP_PROP_FRAME_WIDTH));
    frame_height_ = static_cast<int>(video_.get(cv::CAP_PROP_FRAME_HEIGHT));
    video_fps_ = video_.get(cv::CAP_PROP_FPS);
}   

// Destructor: cleanup resources
FrameLoader::~FrameLoader() {
    if (video_.isOpened()) {
        video_.release();
    }
}

// Load all frames into memory
bool FrameLoader::loadAllFrames() {
    if (!video_.isOpened()) {
        Utils::log("Error: Video not opened");
        return false;
    }

    if (total_frames_ > 0) {
        frame_buffer_.reserve(total_frames_);
    }

    cv::Mat frame;
    while (video_.read(frame)) {
        if (frame.empty()) {
            break;
        }
        frame_buffer_.emplace_back(frame.clone());
    }

    Utils::log("Loaded " + std::to_string(frame_buffer_.size()) + " frames");
    return !frame_buffer_.empty();
}

// Get a specific frame by index
const cv::Mat& FrameLoader::getFrame(size_t index) const {
    

    if (index >= frame_buffer_.size()) {
        Utils::log("Error: Frame index out of bounds");
        // Return first frame as fallback
        return frame_buffer_[0];
    }
    return frame_buffer_[index];
}

// Get current frame
const cv::Mat& FrameLoader::getCurrentFrame() const {
    return getFrame(current_frame_index_);
}

// Move to next frame
bool FrameLoader::nextFrame() {
    if (current_frame_index_ < frame_buffer_.size() - 1) {
        current_frame_index_++;
        return true;
    }
    return false; // At end of video
}

// Reset to first frame
void FrameLoader::reset() {
    current_frame_index_ = 0;
}

