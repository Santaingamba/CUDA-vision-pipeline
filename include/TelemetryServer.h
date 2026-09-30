#ifndef TELEMETRY_SERVER_H
#define TELEMETRY_SERVER_H

#include "TelemetryPacket.h"
#include <string>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class TelemetryServer {
    private:
        int server_fd_;              // Server socket file descriptor
        int client_fd_;              // Client connection file descriptor
        bool running_;               // Server running status    
        int port_;                   // Server port
        std::ofstream log_file_;     // CSV log file
    
    public:
        // Constructor
        TelemetryServer(int port = 5000);

        // Destructor - cleanup
        ~TelemetryServer();

        // Start server and listen for connection
        bool start();

        // Accept client connection
        bool acceptConnection();

        // Receive and process one packet
        bool receivePacket(TelemetryPacket& packet);

        // Run server loop (receive packets continuously)
        void run();

        // Stop server
        void stop();
};

#endif // TELEMETRY_SERVER_H