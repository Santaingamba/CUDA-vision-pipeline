#ifndef TELEMETRY_PACKET_H
#define TELEMETRY_PACKET_H

#include <cstdint>

struct TelemetryPacket {
    uint64_t frame_id;         // Unique identifier for each frame
    double cpu_time_ms;        // Time spent in CPU preprocessing (milliseconds)
    double gpu_time_ms;        // Time spent in GPU processing (milliseconds)
    double total_latency_ms;   // End-to-end latency for this frame (milliseconds)
    double fps;                 // Current frames per second
};

#endif