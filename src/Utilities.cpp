#include "Utilities.h"
#include <iostream>
#include <iomanip>  
#include <chrono>
#include <string>
#include <ctime>

namespace Utils {
    // Get current time in milliseconds (for measuring latency)
    double getCurrentTimeMs() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        return static_cast<double>(millis);
    }
    
    // Log a message with timestamp
    void log(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // Format: [HH:MM:SS.mmm] [LOG] message
        // std::cout <<"[LOG] "  << message << std::endl;
        std::cout << "["
                  << std::put_time(std::localtime(&time_t_now), "%H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [LOG] " << message << std::endl;
    }
    
    // Helper: Calculate elapsed time between two time points
    double getElapsedMs(const std::chrono::high_resolution_clock::time_point& start) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0;
    }
}