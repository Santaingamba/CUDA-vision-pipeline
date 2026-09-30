#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include <vector>
#include <deque>
#include <string>
#include <chrono>

struct FrameMetrics {
    uint64_t frame_id;
    double cpu_time_ms;
    double gpu_time_ms;
    double total_latency_ms;
    double fps;
};

class MetricsCollector {
    private:
        std::vector<FrameMetrics> all_metrics_;                       // Store all frame metrics
        std::deque<double> latency_window_;                           // Rolling window for FPS calculation
        size_t window_size_;                                          // Size of rolling window (e.g., 30 frames)
        std::chrono::high_resolution_clock::time_point start_time_;
        uint64_t frame_count_;

    public:
        // Constructor
        MetricsCollector(size_t window_size = 30);

        // Destructor
        ~MetricsCollector();

        // Start timing for a new frame
        void startFrame();

        // Record a completed frame's metrics
        void recordFrame(uint64_t frame_id,
                         double cpu_time_ms,
                         double gpu_time_ms,
                         double total_latency_ms);
        
        // Calculate current FPS based on rolling window
        double getCurrentFPS() const;

        // Get statistics
        double getAverageLatency() const;
        double getMinLatency() const;
        double getMaxLatency() const;
        double getAverageCPUTime() const;
        double getAverageGPUTime() const;

        // Get total frames processed
        size_t getTotalFrames() const { return frame_count_; }

        // Export metrics to CSV
        bool exportToCSV(const std::string& filename) const;

        // Print summary to console
        void printSummary() const;

        // Clear all metrics
        void reset();
};

#endif // METRICS_COLLECTOR_H