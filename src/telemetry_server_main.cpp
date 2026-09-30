#include "TelemetryServer.h"
#include "Utilities.h"
#include <iostream>
#include <csignal>

TelemetryServer* server_ptr = nullptr;

void signalHandler(int signal) {
    if (server_ptr) {
        Utils::log("\nShutting down server...");
        server_ptr->stop();
    }
    exit(0);
}

int main() {
    Utils::log("=== Telemetry Server ===\n");

    TelemetryServer server(5000);
    server_ptr = &server;

    // Handle Ctrl+C gracefully
    signal(SIGINT, signalHandler);

    if (!server.start()) {
        return 1;
    }

    if (!server.acceptConnection()) {
        return 1;
    }

    server.run();
    
    return 0;
}