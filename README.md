# CUDA Vision Pipeline

A C++17 real-time video processing pipeline that uses **CUDA GPU acceleration** for Sobel edge detection, with CPU-based preprocessing, rolling performance metrics, and optional TCP/IP telemetry streaming.

## Overview

This project implements an end-to-end frame processing pipeline designed for low-latency computer vision workloads. Video frames are loaded from file via OpenCV, preprocessed on the CPU, then processed on the GPU using a custom CUDA kernel. Performance metrics are tracked per-frame and can be streamed to a remote telemetry server over TCP.

The pipeline demonstrates practical GPU-accelerated image processing with explicit control over host-device memory transfers, CUDA kernel configuration, and CPU-GPU coordination.

```
Video File → FrameLoader → CPU Preprocessing → CUDA GPU Processing → Metrics / Telemetry
```

## Features

- **Video frame loading** — OpenCV-based video I/O with full in-memory frame buffering
- **CPU grayscale conversion** — Configurable thread pool for parallel preprocessing across frame batches
- **CUDA Sobel edge detection** — Custom GPU kernel using constant memory for filter coefficients
- **Performance metrics** — Per-frame CPU/GPU timing, rolling-window FPS calculation, CSV export
- **TCP/IP telemetry** — Optional binary-protocol client/server for real-time remote monitoring
- **Command-line interface** — Configurable frame count, thread count, video path, and telemetry endpoint

## Architecture

```
┌──────────────────┐
│   FrameLoader    │  OpenCV VideoCapture → in-memory frame buffer
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ CPUPreprocessor  │  BGR → Grayscale conversion (multithreaded for batches)
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  GPUProcessor    │  Host→Device transfer → CUDA Sobel kernel → Device→Host transfer
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ MetricsCollector │  Per-frame timing, rolling FPS, statistics, CSV export
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ TelemetryClient  │  Binary TCP streaming to remote server (optional)
└──────────────────┘
```

### Components

| Component | File(s) | Description |
|-----------|---------|-------------|
| **FrameLoader** | `FrameLoader.h/cpp` | Opens video file, reads all frames into a `std::vector<cv::Mat>` buffer |
| **CPUPreprocessor** | `CPUPreprocessor.h/cpp` | Converts BGR frames to grayscale; supports multithreaded batch processing via `std::thread` pool |
| **GPUProcessor** | `GPUProcessor.h`, `GPUProcessor.cu` | Manages GPU memory allocation, host↔device transfers, and launches the Sobel kernel |
| **MetricsCollector** | `MetricsCollector.h/cpp` | Tracks per-frame CPU time, GPU time, total latency; computes rolling-window FPS and exports to CSV |
| **TelemetryClient** | `TelemetryClient.h/cpp` | Connects to a TCP server and sends `TelemetryPacket` structs as binary data |
| **TelemetryServer** | `TelemetryServer.h/cpp`, `telemetry_server_main.cpp` | Standalone TCP server that receives packets, logs to console and CSV |
| **Utilities** | `Utilities.h/cpp` | Timestamped logging, high-resolution elapsed-time measurement |

## Processing Pipeline

The main pipeline (`src/main.cpp`) processes frames sequentially:

1. **Load** — `FrameLoader` reads the video file and buffers all frames in memory
2. **GPU warm-up** — A single frame is processed to initialize the CUDA context
3. **Per-frame loop:**
   - Load frame from buffer
   - **CPU:** Convert to grayscale (`cv::cvtColor` BGR→GRAY)
   - **GPU:** Copy grayscale frame to device → run Sobel edge detection kernel → copy result back to host
   - Record CPU time, GPU time (kernel + transfer), and total latency
   - Optionally send a telemetry packet over TCP
4. **Summary** — Print aggregate statistics and export per-frame metrics to CSV

## CUDA Implementation

### Sobel Edge Detection Kernel

The GPU kernel (`src/GPUProcessor.cu`) implements a standard 3×3 Sobel operator:

- **Thread configuration:** 2D blocks of 16×16 threads; grid sized to cover the full image
- **Constant memory:** Sobel Gx and Gy filter coefficients stored in CUDA `__constant__` memory for fast cached access across all threads
- **Boundary handling:** Border pixels (1-pixel margin) are set to 0; interior pixels compute the gradient magnitude `√(Gx² + Gy²)` clamped to [0, 255]
- **Memory management:** Device buffers are allocated once and reused for all frames of the same resolution; reallocation occurs only on resolution change

### Memory Transfer Timing

Host→Device and Device→Host transfers are timed separately from kernel execution, enabling bottleneck analysis between compute and data transfer.

## CPU Implementation

- **Grayscale conversion** uses OpenCV's `cv::cvtColor(BGR2GRAY)`
- **Batch threading** (`processFrameBatch`) partitions frames across N worker threads using `std::thread`, with each thread processing a contiguous range — this path is used when processing frame batches, while the main pipeline processes frames individually
- Thread count is configurable via `--threads` CLI argument

## Performance

Performance metrics are collected by `MetricsCollector`:

- **Per-frame:** CPU preprocessing time, GPU processing time (kernel + transfer), total end-to-end latency
- **Rolling window:** FPS calculated over a configurable sliding window (default: 30 frames)
- **Aggregate:** Average/min/max latency, average CPU time, average GPU time
- **Export:** Full per-frame metrics written to CSV for offline analysis

> **Note:** Performance figures depend on hardware (GPU model, CPU, memory bandwidth), input resolution, CUDA toolkit version, and runtime configuration. Results should be treated as environment-specific measurements.

## Requirements

| Dependency | Version |
|-----------|---------|
| C++ compiler | C++17 support required (g++ 7+, MSVC 2017+) |
| CMake | 3.18+ |
| CUDA Toolkit | 11.0+ (with `nvcc`) |
| NVIDIA GPU | Compute capability supported by your CUDA toolkit |
| OpenCV | 4.x |
| Platform | Linux or WSL (telemetry uses POSIX sockets) |

## Build

### Linux / WSL

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential cmake libopencv-dev

# CUDA Toolkit: https://developer.nvidia.com/cuda-downloads

# Clone and build
git clone https://github.com/Santaingamba/CUDA-vision-pipeline.git
cd CUDA-vision-pipeline
mkdir build && cd build
cmake ..
make

# Verify
./camera_pipeline --help
```

### Windows (with CUDA + OpenCV configured)

```bash
git clone https://github.com/Santaingamba/CUDA-vision-pipeline.git
cd CUDA-vision-pipeline
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

> **Note:** The telemetry client/server components use POSIX sockets (`<sys/socket.h>`) and require Linux or WSL. The core pipeline (frame loading, CPU preprocessing, GPU processing, metrics) can be adapted for native Windows by replacing the socket layer with Winsock.

## Usage

### Basic Pipeline

```bash
# Default: 100 frames, single-threaded, using examples/sample_video.mp4
./camera_pipeline

# Custom frame count
./camera_pipeline --frames 200

# Multi-threaded CPU preprocessing
./camera_pipeline --threads 4 --frames 100

# Custom video file
./camera_pipeline --video /path/to/video.mp4 --threads 4
```

### With Telemetry

```bash
# Terminal 1 — start telemetry server
./telemetry_server

# Terminal 2 — run pipeline with telemetry enabled
./camera_pipeline --telemetry 127.0.0.1:5000 --frames 150 --threads 4
```

### CLI Reference

| Option | Description | Default |
|--------|-------------|---------|
| `--frames N` | Number of frames to process | 100 |
| `--threads N` | CPU threads for preprocessing | 1 |
| `--video PATH` | Input video file path | `../examples/sample_video.mp4` |
| `--telemetry IP:PORT` | Enable telemetry streaming | disabled |
| `--help` | Show usage information | — |

## Project Structure

```
CUDA-vision-pipeline/
├── CMakeLists.txt              # Build configuration
├── README.md
├── .gitignore
├── include/
│   ├── CPUPreprocessor.h       # CPU preprocessing interface
│   ├── FrameLoader.h           # Video frame loading interface
│   ├── GPUProcessor.h          # GPU processing + kernel declaration
│   ├── MetricsCollector.h      # Performance metrics interface
│   ├── TelemetryClient.h       # TCP client interface
│   ├── TelemetryPacket.h       # Binary telemetry packet struct
│   ├── TelemetryServer.h       # TCP server interface
│   └── Utilities.h             # Logging and timing utilities
├── src/
│   ├── main.cpp                # Main pipeline entry point
│   ├── CPUPreprocessor.cpp     # CPU grayscale + threading
│   ├── FrameLoader.cpp         # OpenCV video loading
│   ├── GPUProcessor.cu         # CUDA Sobel kernel + GPU memory management
│   ├── MetricsCollector.cpp    # Metrics collection + CSV export
│   ├── TelemetryClient.cpp     # TCP client implementation
│   ├── TelemetryServer.cpp     # TCP server implementation
│   ├── telemetry_server_main.cpp  # Telemetry server entry point
│   └── Utilities.cpp           # Logging and timing implementation
├── examples/
│   └── sample_video.mp4        # Sample input video
├── scripts/
│   └── plot_metrics.py         # Metrics visualization script (placeholder)
└── data/                       # Runtime output directory (CSV exports)
```

## Sample Video

The included sample video (`examples/sample_video.mp4`) is sourced from [Pexels](https://www.pexels.com/video/vehicle-on-highway-with-dash-cam-4608285/) (free license).

## Author

**Chaphamayum Santaingamba**

## Known Limitations

- Telemetry networking uses POSIX sockets — requires Linux or WSL
- Frames are processed sequentially in the main pipeline (no CUDA streams or async overlap)
- All video frames are loaded into RAM before processing begins
- The `processFrameBatch` method is declared in `GPUProcessor.h` but not yet implemented
- `scripts/plot_metrics.py` is a placeholder and not yet implemented
