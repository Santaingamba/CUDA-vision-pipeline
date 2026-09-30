#include "TelemetryClient.h"
#include "TelemetryPacket.h"
#include "Utilities.h"
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Constructor
TelemetryClient::TelemetryClient(const std::string& server_ip, int server_port)
    : socket_fd_(-1),
      connected_(false),
      server_ip_(server_ip),
      server_port_(server_port)
{
    Utils::log("TelemetryClient initialized (target: " + server_ip_ + ":" +
                std::to_string(server_port_) + ")");
}
    
// Destructor - close connection
TelemetryClient::~TelemetryClient() {
    disconnect();
}

// Connect to server
bool TelemetryClient::connect() {
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        Utils::log("Error: Failed to create socket");
        return false;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_);

    // Convert IP address
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
        Utils::log("Error: Invalid IP address");
        close(socket_fd_);
        return false;
    }

    // Connect to server
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Utils::log("Error: Connection failed");
        close(socket_fd_);
        return false;
    }

    connected_ = true;
    Utils::log("Connected to telemetry server");
    return true;
}

// Send telemetry packet
bool TelemetryClient::sendPacket(const TelemetryPacket& packet) {
    if (!connected_) {
        Utils::log("Error: Not connected to server");
        return false;
    }

    // Send the entire struct as binary data
    ssize_t bytes_sent = send(socket_fd_, &packet, sizeof(TelemetryPacket), 0);

    if (bytes_sent < 0) {
        Utils::log("Error: Failed to send packet");
        return false;
    }

    return true;
}

// Disconnect from server
void TelemetryClient::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
    Utils::log("Disconnected from telemetry server");
}

