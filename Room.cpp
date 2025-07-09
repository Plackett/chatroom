#include "Room.h"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <algorithm>

// Room.cpp
// Implementation of the Room class, handling both server and client logic.

// --- Public Methods ---

void Room::startServer(const int port) {
    int server_fd;
    sockaddr_in address{};

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of address and port
    constexpr int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    setupAddress(address, port);

    // Bind the socket to the network address and port
    if (bind(server_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << port << ". Waiting for connections..." << std::endl;
    serverLoop(server_fd);
}

void Room::joinRoom(const std::string& ipAddress, int port) {
    int sock = 0;
    sockaddr_in serv_addr{};

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "\n Socket creation error \n";
        return;
    }

    setupAddress(serv_addr, port, ipAddress);

    // Connect to the server
    if (connect(sock, reinterpret_cast<const sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        std::cerr << "\nConnection Failed \n";
        return;
    }

    std::cout << "Successfully connected to the room!" << std::endl;
    std::cout << "You can start sending messages. Type '/quit' to exit." << std::endl;
    clientLoop(sock);
}


// --- Private Server Methods ---

void Room::serverLoop(const int server_fd) {
    fd_set master_set, working_set;
    int max_sd = server_fd;

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    while (true) {
        working_set = master_set;
        // Wait for an activity on one of the sockets
        if (select(max_sd + 1, &working_set, nullptr, nullptr, nullptr) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                // If it's the listening socket, accept a new connection
                if (i == server_fd) {
                    int new_socket;
                    sockaddr_in client_address{};
                    socklen_t address_length = sizeof(client_address);
                    if ((new_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&client_address), &address_length)) < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }

                    std::cout << "New connection, socket fd is " << new_socket
                              << ", ip is : " << inet_ntoa(client_address.sin_addr)
                              << ", port : " << ntohs(client_address.sin_port) << std::endl;

                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd) {
                        max_sd = new_socket;
                    }
                    client_sockets.push_back(new_socket);
                } else { // It's a client socket, handle incoming message
                    char buffer[1024] = {0};

                    if (const int value_read = static_cast<int>(read(i, buffer, 1024)); value_read <= 0) {
                        // Client disconnected
                        sockaddr_in client_addr{};
                        socklen_t addr_len = sizeof(client_addr);
                        getpeername(i, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
                        std::cout << "Host disconnected, ip " << inet_ntoa(client_addr.sin_addr)
                                  << ", port " << ntohs(client_addr.sin_port) << std::endl;
                        close(i);
                        FD_CLR(i, &master_set);
                        // Remove from client list
                        client_sockets.erase(std::remove(client_sockets.begin(), client_sockets.end(), i), client_sockets.end());
                    } else {
                        // Broadcast the message to other clients
                        broadcastMessage(buffer, i);
                    }
                }
            }
        }
    }
}


void Room::broadcastMessage(const char* message, const int sender_fd) const {
    for (const int client_fd : client_sockets) {
        if (client_fd != sender_fd) {
            send(client_fd, message, strlen(message), 0);
        }
    }
}


// --- Private Client Methods ---

void receiveMessages(const int sock) {
    char buffer[1024] = {0};
    while (true) {
        if (const int value_read = static_cast<int>(read(sock, buffer, 1024)); value_read > 0) {
            std::cout << "\n> " << buffer << std::endl;
            memset(buffer, 0, sizeof(buffer)); // Clear buffer
        } else {
             std::cout << "\nServer disconnected." << std::endl;
             close(sock);
             exit(0);
        }
    }
}

void Room::clientLoop(int client_fd) {
    // Start a separate thread to receive messages
    std::thread receiver_thread(receiveMessages, client_fd);
    receiver_thread.detach(); // Let the thread run independently

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "/quit") {
            break;
        }
        send(client_fd, message.c_str(), message.length(), 0);
    }
    close(client_fd);
}

// --- Private Utility Methods ---

void Room::setupAddress(sockaddr_in& address, const int port, const std::string& ip) {
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (!ip.empty()) {
        // Client-side: convert IP from text to binary
        if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
            std::cerr << "\nInvalid address/ Address not supported \n";
            exit(EXIT_FAILURE);
        }
    } else {
        // Server-side: accept connections on any IP
        address.sin_addr.s_addr = INADDR_ANY;
    }
}