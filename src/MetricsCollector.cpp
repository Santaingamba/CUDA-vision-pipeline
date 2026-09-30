#include "MetricsCollector.h"
#include "Utilities.h"
#include <vector>
#include <deque>
#include <string>
#include <chrono>
#include <iostream>
#include <fstream>


// Constructor
MetricsCollector::MetricsCollector(size_t window_size)
    : window_size_(window_size),
      frame_count_(0)
{
    all_metrics_.reserve(1000); // Pre-allocate for performance
    Utils::log("MetricsCollector initialized with window size: " + 
               std::to_string(window_size_));
}

// Destructor
MetricsCollector::~MetricsCollector() {

}

// Start timing for a new frame
void MetricsCollector::startFrame() {
    start_time_ = std::chrono::high_resolution_clock::now();
}

// Record a completed frame's metrics
void MetricsCollector::recordFrame(uint64_t frame_id,
                    double cpu_time_ms,
                    double gpu_time_ms,
                    double total_latency_ms)
{
    // Update rolling window for FPS
    latency_window_.push_back(total_latency_ms);
    if (latency_window_.size() > window_size_) {
        latency_window_.pop_front(); // Keep window size fixed
    }

    // Calculate current FPS
    double fps = getCurrentFPS();

    // Store metrics
    FrameMetrics metrics;
    metrics.frame_id = frame_id;
    metrics.cpu_time_ms = cpu_time_ms;
    metrics.gpu_time_ms = gpu_time_ms;
    metrics.total_latency_ms = total_latency_ms;
    metrics.fps = fps;

    all_metrics_.push_back(metrics);
    frame_count_++;
}

// Calculate current FPS based on rolling window
double MetricsCollector::getCurrentFPS() const {
    if (latency_window_.empty()) {
        return 0.0;
    }

    // Sum all latencies in window
    double total_latency = 0.0;
    for (double latency: latency_window_) {
        total_latency += latency;
    }

    // FPS = num_frames / total_time_in_seconds
    // total_time_in_seconds = total_latency_ms / 1000
    return (latency_window_.size() / total_latency) * 1000.0;
}

// Get statistics
double MetricsCollector::getAverageLatency() const {
    if (all_metrics_.empty()) return 0.0;

    double latency_time = 0.0;
    for (const auto& m: all_metrics_) {
        latency_time += m.total_latency_ms;
    }
    return latency_time / all_metrics_.size();
}

double MetricsCollector::getMinLatency() const {
    if (all_metrics_.empty()) return 0.0;

    double min_latency = all_metrics_[0].total_latency_ms;
    for (const auto& m: all_metrics_) {
        if (m.total_latency_ms < min_latency) {
            min_latency = m.total_latency_ms;
        }   
    }
    return min_latency;
}

double MetricsCollector::getMaxLatency() const {
    if (all_metrics_.empty()) return 0.0;

    double max_latency = all_metrics_[0].total_latency_ms;
    for (const auto& m: all_metrics_) {
        if (m.total_latency_ms > max_latency) {
            max_latency = m.total_latency_ms;
        }   
    }
    return max_latency;
}

double MetricsCollector::getAverageCPUTime() const {
    if (all_metrics_.empty()) return 0.0;

    double cpu_time = 0.0;
    for (const auto& m: all_metrics_) {
        cpu_time += m.cpu_time_ms;
    }
    return cpu_time / all_metrics_.size();
}

double MetricsCollector::getAverageGPUTime() const {
    if (all_metrics_.empty()) return 0.0;

    double gpu_time = 0.0;
    for (const auto& m: all_metrics_) {
        gpu_time += m.gpu_time_ms;
    }
    return gpu_time / all_metrics_.size();
}

// Export metrics to CSV
bool MetricsCollector::exportToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Utils::log("Error: Could not open file " + filename);
        return false;
    }

    // Write header
    file << "frame_id,cpu_time_ms,gpu_time_ms,total_latency_ms,fps\n";

    // Write data
    for (const auto& m: all_metrics_) {
        file << m.frame_id << ","
             << m.cpu_time_ms << ","
             << m.gpu_time_ms << ","
             << m.total_latency_ms << ","
             << m.fps << "\n";
    }

    file.close();
    Utils::log("Metrics exported to " + filename);
    return true;
}

// Print summary to console
void MetricsCollector::printSummary() const {
    std::cout << "\n=== Performance Summary ===" << std::endl;
    std::cout << "  Total frames processed:   " << frame_count_ << std::endl;
    std::cout << "  Average latency:          " << getAverageLatency() << " ms" << std::endl;
    std::cout << "  Min latency:              " << getMinLatency() << " ms" << std::endl;
    std::cout << "  Max latency:              " << getMaxLatency() << " ms" << std::endl;
    std::cout << "  Average CPU time:         " << getAverageCPUTime() << " ms" << std::endl;
    std::cout << "  Average GPU time:         " << getAverageGPUTime() << " ms" << std::endl;
    std::cout << "  Average FPS:              " << (1000.0 / getAverageLatency()) << std::endl;

}

// Clear all metrics
void MetricsCollector::reset() {
    all_metrics_.clear();
    latency_window_.clear();
    frame_count_ = 0;
}
