cmake_minimum_required(VERSION 3.18)
project(CameraPipeline LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 14)

# Find packages
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/FrameLoader.cpp
    src/CPUPreprocessor.cpp
    src/MetricsCollector.cpp
    src/TelemetryClient.cpp
    src/Utilities.cpp
)

set(CUDA_SOURCES
    src/GPUProcessor.cu
)

# Main pipeline executable
add_executable(camera_pipeline
    src/main.cpp
    ${SOURCES}
    ${CUDA_SOURCES}
)

target_link_libraries(camera_pipeline
    ${OpenCV_LIBS}
    pthread
)

# Telemetry server executable
add_executable(telemetry_server
    src/telemetry_server_main.cpp
    src/TelemetryServer.cpp
    src/Utilities.cpp
)

target_link_libraries(telemetry_server
    pthread
)

# Set output directory
set_target_properties(camera_pipeline telemetry_server
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
