#include "MetricsCollector.h"
#include "Utilities.h"
#include "FrameLoader.h"
#include "CPUPreprocessor.h"
#include "GPUProcessor.h"
#include "TelemetryPacket.h"
#include "TelemetryClient.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <opencv2/opencv.hpp>

void printUsage(const char* program_name) {
    std::cout << "\nUsage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --frames N          Number of frames to process (default: 100)\n";
    std::cout << "  --threads N         CPU threads for preprocessing (default: 1)\n";
    std::cout << "  --telemetry IP:PORT Enable telemetry (default: disabled)\n";
    std::cout << "  --video PATH        Video file path (default: ../examples/sample_video.mp4\n";
    std::cout << "  --help              Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " --frames 200 --threads 4\n";
    std::cout << "  " << program_name << " --telemetry 127.0.0.1:5000 --frames 500\n";
    std::cout << "  " << program_name << " --video /path/to/video.mp4 --threads 8\n";
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Default parameters
    int num_frames = 100;
    int cpu_threads = 1;
    std::string video_path = "../examples/sample_video.mp4";
    bool enable_telemetry = false;
    std::string telemetry_ip = "127.0.0.1";
    int telemetry_port = 5000;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            num_frames = std::atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            cpu_threads = std::atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--video") == 0 && i + 1 < argc) {
            video_path = argv[++i];
        }
        else if (strcmp(argv[i], "--telemetry") == 0 && i + 1 < argc) {
            enable_telemetry = true;
            std::string addr = argv[++i];
            size_t colon_pos = addr.find(':');
            if (colon_pos != std::string::npos) {
                telemetry_ip = addr.substr(0, colon_pos);
                telemetry_port = std::atoi(addr.substr(colon_pos + 1).c_str());
            }
        }
        else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Print configuration
    Utils::log("=== Camera Processing Pipeline ===");
    std::cout << std::endl;
    Utils::log("Configuration:");
    Utils::log("  Video: " + video_path);
    Utils::log("  Frames to process: " + std::to_string(num_frames));
    Utils::log("  CPU threads: " + std::to_string(cpu_threads));
    if (enable_telemetry) {
        Utils::log("  Telemetry: " + telemetry_ip + ":" + std::to_string(telemetry_port));
    } else {
        Utils::log("  Telemetry: disabled");
    }
    std::cout << std::endl;

    // Telemetry Client
    TelemetryClient* telemetry = nullptr;
    if (enable_telemetry) {
        telemetry = new TelemetryClient(telemetry_ip, telemetry_port);
        if (!telemetry->connect()) {
            Utils::log("Warning: Could not connect to telemetry server");
            delete telemetry;
            telemetry = nullptr;
        }
    }

    // Load video
    FrameLoader loader(video_path);
    Utils::log("Loading video frames...");
    if (!loader.loadAllFrames()) {
        Utils::log("Failed to load frames");
        if (telemetry) delete telemetry;
        return 1;
    }

    // Limit frames to available
    num_frames = std::min(num_frames, loader.getTotalFrames());
    Utils::log("Processing " + std::to_string(num_frames) + " of " +
                std::to_string(loader.getTotalFrames()) + " total frames");
    std::cout << std::endl;
    
    // Initialize processors
    CPUPreprocessor cpu_proc(cpu_threads);
    GPUProcessor gpu;
    MetricsCollector metrics(30);  // 30-frame rolling window

    // Warm-up GPU
    cv::Mat first_frame = loader.getFrame(0);
    cv::Mat gray_warmup;
    cv::cvtColor(first_frame, gray_warmup, cv::COLOR_BGR2GRAY);

    Utils::log("Warming up GPU...");
    double dummy_k, dummy_t;
    gpu.processSingleFrame(gray_warmup, dummy_k, dummy_t);
    Utils::log("GPU ready");

    std::cout << std::endl;

    Utils::log("Processing Pipeline: FrameLoader -> CPU Preprocessing -> GPU Processing -> Telemetry Metrics");
    if (telemetry) {
        Utils::log("Telemetry: enabled");
    }
    std::cout << std::endl;

    // Process frames
    for (int i = 0; i < num_frames; i++) {
        // Start frame timing
        auto frame_start = std::chrono::high_resolution_clock::now();

        // 1. Load frame
        cv::Mat frame = loader.getFrame(i);

        // 2. CPU Preprocessing (grayscale conversion)
        auto cpu_start = std::chrono::high_resolution_clock::now();
        cv::Mat processed = cpu_proc.processSingleFrame(frame);
        double cpu_time = Utils::getElapsedMs(cpu_start);
        
        // 3. GPU Processing (Sobel edge detection)
        double kernel_time, transfer_time;
        cv::Mat result = gpu.processSingleFrame(processed, kernel_time, transfer_time);
        double gpu_time = kernel_time + transfer_time;

        // 4. Total Latency
        double total_time = Utils::getElapsedMs(frame_start);

        // 5. Record metrics
        metrics.recordFrame(i, cpu_time, gpu_time, total_time);
        
        // 6. Send Telemtry
        if (telemetry) {
            TelemetryPacket packet;
            packet.frame_id = i;
            packet.cpu_time_ms = cpu_time;
            packet.gpu_time_ms = gpu_time;
            packet.total_latency_ms = total_time;
            packet.fps = metrics.getCurrentFPS();
            telemetry->sendPacket(packet);
        }
    }

    std::cout << std::endl;

    // Print summary
    metrics.printSummary();

    // Export to CSV
    metrics.exportToCSV("../data/metrics.csv");

    // Cleanup
    if (telemetry) {
        delete telemetry;
    }

    // Destructor will handle disconnect via RAII
    
    return 0;
}