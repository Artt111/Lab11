#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

class UDPSocket {
private:
    int sock;
    struct sockaddr_in server_address;

public:
    // Constructor: Initialize the UDP socket
    UDPSocket(const std::string &server_ip, int port) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Error creating socket");
        }

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0) {
            throw std::runtime_error("Invalid server IP address");
        }

        struct timeval timeout;
        timeout.tv_sec = 5;  // 5-second timeout
        timeout.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            throw std::runtime_error("Error setting socket timeout");
        }
    }

    // Destructor: Clean up the socket
    ~UDPSocket() {
        close(sock);
    }

    // Send a datagram
    ssize_t send_datagram(const std::string &message) {
        ssize_t bytes_sent = sendto(sock, message.c_str(), message.size(), 0,
                                    (struct sockaddr *)&server_address, sizeof(server_address));
        if (bytes_sent < 0) {
            std::cerr << "Error: Failed to send datagram." << std::endl;
            return -1;
        }
        return bytes_sent;
    }

    // Receive a datagram
    std::pair<std::string, bool> recv_datagram(size_t bufsize) {
        char buffer[bufsize];
        socklen_t addr_len = sizeof(server_address);
        ssize_t bytes_received = recvfrom(sock, buffer, bufsize - 1, 0,
                                          (struct sockaddr *)&server_address, &addr_len);

        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "Error: Timeout while waiting for a response." << std::endl;
            } else {
                std::cerr << "Error receiving datagram." << std::endl;
            }
            return {"", false};
        }

        buffer[bytes_received] = '\0'; // Null-terminate the string
        return {std::string(buffer), true};
    }
};

int main() {
    const std::string SERVER = "127.0.0.1";
    const int PORT = 8888;
    const size_t BUFLEN = 1024;

    try {
        UDPSocket client_sock(SERVER, PORT);
        std::cout << "Client initialized. Server address: " << SERVER << ", port: " << PORT << std::endl;

        while (true) {
            std::cout << "Enter message (or type 'exit' to quit): ";
            std::string message;
            std::getline(std::cin, message);

            if (message.empty()) {
                std::cout << "Please enter a non-empty message." << std::endl;
                continue;
            }

            if (message == "exit") {
                std::cout << "Client shutting down." << std::endl;
                break;
            }

            if (client_sock.send_datagram(message) == -1) {
                std::cerr << "Failed to send message." << std::endl;
                continue;
            }

            auto [response, success] = client_sock.recv_datagram(BUFLEN);
            if (success) {
                std::cout << "Received from server: " << response << std::endl;
            } else {
                std::cout << "No response from server." << std::endl;
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
