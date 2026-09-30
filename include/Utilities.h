#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <chrono>

namespace Utils {
    // Get current time in milliseconds (for measuring latency)
    double getCurrentTimeMs();

    // Log a message with timestamp
    void log(const std::string& message);

    // Helper: Calculate elapsed time between two time points
    double getElapsedMs(const std::chrono::high_resolution_clock::time_point& start);
}


#endif // UTILITIES_H