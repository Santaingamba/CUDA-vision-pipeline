#include "TelemetryServer.h"
#include "Utilities.h"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


// Constructor
TelemetryServer::TelemetryServer(int port)
    : server_fd_(-1),
      client_fd_(-1),
      running_(false),
      port_(port)
{
    Utils::log("TelemetryServer initialized on port " + std::to_string(port_));
}

// Destructor - cleanup
TelemetryServer::~TelemetryServer() {
    if (client_fd_ >= 0) {
        close(client_fd_);
    }
    if (server_fd_ >= 0) {
        close(server_fd_);
    }
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

// Start server and listen for connection
bool TelemetryServer::start() {
    // Create socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        Utils::log("Error: Failed to create server socket");
        return false;
    }

    // Allow address reuse (useful for quick restarts)
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Setup server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(port_);

    // Bind socket to port
    if (bind(server_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Utils::log("Error: Failed to bind to port " + std::to_string(port_));
        close(server_fd_);
        return false;
    }

    // Listen for connections
    if (listen(server_fd_, 1) < 0) {
        Utils::log("Error: Failed to listen");
        close(server_fd_);
        return false;
    }

    running_ = true;
    Utils::log("Telemetry server listening on port " + std::to_string(port_));
    return true;
}

// Accept client connection
bool TelemetryServer::acceptConnection() {
    Utils::log("Waiting for client connection...");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    client_fd_ = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd_ < 0) {
        Utils::log("Error: Failed to accept connection");
        return false;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    Utils::log("Client connected from " + std::string(client_ip));

    return true;
}

// Receive and process one packet
bool TelemetryServer::receivePacket(TelemetryPacket& packet) {
    ssize_t bytes_received = recv(client_fd_, &packet, sizeof(TelemetryPacket), 0);

    if (bytes_received < 0) {
        Utils::log("Error: Failed to receive packet");
        return false;
    }

    if (bytes_received == 0) {
        Utils::log("Client disconnected");
        return false;
    }

    return true;
}

// Run server loop (receive packets continuously)
void TelemetryServer::run() {
    // Open CSV log file
    log_file_.open("../data/telemetry_log.csv");
    if (log_file_.is_open()) {
        log_file_ << "frame_id,cpu_time_ms,gpu_time_ms,total_latency_ms,fps\n";
    }

    Utils::log("Server running. Press Ctrl+C to stop.\n");

    while (running_) {
        TelemetryPacket packet;

        if (receivePacket(packet)) {
            // Log to console
            std::cout << "[Frame " << packet.frame_id << "] "
                      << "CPU: " << packet.cpu_time_ms << "ms, "
                      << "GPU: " << packet.gpu_time_ms << "ms, "
                      << "Total: " << packet.total_latency_ms << "ms, "
                      << "FPS: " << packet.fps << std::endl;

            // Log to CSV
            if (log_file_.is_open()) {
                log_file_ << packet.frame_id << ","
                          << packet.cpu_time_ms << ","
                          << packet.gpu_time_ms << ","
                          << packet.total_latency_ms << ","
                          << packet.fps << "\n";
                log_file_.flush(); // Ensure data is written immediately
            }
        } else {
            break; // Client disconnected or error
        }
    }

    if (log_file_.is_open()) {
        log_file_.close();
        Utils::log("Telemetry log saved to ../data/telemetry_log.csv");
    }
}

// Stop server
void TelemetryServer::stop() {
    running_ = false;

    if (client_fd_ >= 0) {
        close(client_fd_);
        client_fd_ = -1;
    }

    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }

    Utils::log("Telemetry server stopped");
}
