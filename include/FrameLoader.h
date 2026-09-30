#ifndef FRAME_LOADER_H
#define FRAME_LOADER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class FrameLoader {
    private:
        cv::VideoCapture video_;              // OpenCV video capture object
        std::vector<cv::Mat> frame_buffer_;   // Store all frames in memory
        size_t current_frame_index_;          // Which frame we're on
        int total_frames_;                    // Total number of frames
        int frame_width_;                     // Frame width in pixels
        int frame_height_;                    // Frame height in pixels
        double video_fps_;                    

    public:
        // Constructor: open video file
        FrameLoader(const std::string& video_path);

        // Destructor: cleanup resources
        ~FrameLoader();

        // Load all frames into memory
        bool loadAllFrames();

        // Get a specific frame by index
        const cv::Mat& getFrame(size_t index) const;

        // Get current frame
        const cv::Mat& getCurrentFrame() const;

        // Move to next frame
        bool nextFrame();

        // Reset to first frame
        void reset();

        // Getters for metadata
        int getTotalFrames() const { return total_frames_; }
        int getWidth() const { return frame_width_; }
        int getHeight() const { return frame_height_; }
        double getFPS() const { return video_fps_; }
        size_t getCurrentIndex() const { return current_frame_index_; }
};

#endif // FRAME_LOADER_H