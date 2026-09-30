#ifndef TELEMETRY_CLIENT_H
#define TELEMETRY_CLIENT_H

#include "TelemetryPacket.h"
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class TelemetryClient {
    private:
        int socket_fd_;           // Socket file descriptor
        bool connected_;          // Connection status
        std::string server_ip_;   // Server IP address
        int server_port_;         // Server port

    public:
        // Constructor
        TelemetryClient(const std::string& server_ip = "127.0.0.1",
                        int server_port = 5000);
            
        // Destructor - close connection
        ~TelemetryClient();

        // Connect to server
        bool connect();

        // Send telemetry packet
        bool sendPacket(const TelemetryPacket& packet);

        // Disconnect from server
        void disconnect();

        // Check if connected
        bool isConnected() const { return connected_; }
};

#endif // TELEMETRY_CLIENT_H
