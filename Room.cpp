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
#include <atomic>
#include <ranges>

// Room.cpp
// Implementation of the Room class, handling both server and client logic.

// Atomic boolean to safely control the server's main loop from another thread.
std::atomic<bool> server_running{true};

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

    std::cout << "🚀 Server started on port " << port << ". Waiting for connections..." << std::endl;
    std::cout << "💡 As the host, type '/help' for a list of commands." << std::endl;

    char host_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, host_ip, INET_ADDRSTRLEN);
    clients[0] = ClientInfo{"HOST", host_ip, std::to_string(port), "👑", 5};

    // Start a detached thread to handle input from the host.
    std::thread host_input_thread(&Room::hostInputLoop, this, server_fd);
    host_input_thread.detach();

    serverLoop(server_fd);
}

void Room::joinRoom(const std::string& ipAddress, const int port) {
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

    // The initial welcome message is now sent by the server upon connection.
    clientLoop(sock);
}


// --- Private Server Methods ---

void Room::hostInputLoop(const int server_fd) {
    std::string message;
    while (server_running) {
        std::getline(std::cin, message);

        if (!server_running) break;

        if (message == "/close") {
            std::cout << "🔒 Closing the room..." << std::endl;
            broadcastMessage("🔒 Host has closed the room. Goodbye!", -1);
            server_running = false;
            shutdown(server_fd, SHUT_RD);
            close(server_fd);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            exit(0);
        } else if (message.at(0) == '/') {
            handleCommand(0,message);
        } else if (!message.empty()) {
            std::string host_message = clients[0].prefix + " [" + clients[0].nickname + "]: " + message;
            broadcastMessage(host_message.c_str(), -1);
        }
    }
}

void Room::serverLoop(const int server_fd) {
    fd_set master_set, working_set;
    int max_sd = server_fd;

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    while (server_running) {
        working_set = master_set;
        struct timeval timeout = {1, 0};

        if (select(max_sd + 1, &working_set, nullptr, nullptr, &timeout) < 0) {
            if(server_running) perror("select error");
            break;
        }

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                if (i == server_fd) { // New connection
                    sockaddr_in client_address{};
                    socklen_t address_length = sizeof(client_address);
                    int new_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&client_address), &address_length);

                    if (new_socket < 0) {
                        if (server_running) perror("accept");
                        continue;
                    }

                    std::string client_ip = inet_ntoa(client_address.sin_addr);
                    std::string default_nick = "user" + std::to_string(new_socket);

                    std::cout << "✅ New connection from " << client_ip << " on socket " << new_socket << std::endl;

                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd) max_sd = new_socket;

                    clients[new_socket] = {default_nick, client_ip, std::to_string(ntohs(client_address.sin_port)), "🗣️", 0};

                    std::string welcome = "🎉 Welcome! Your default name is " + default_nick + ". Type '/help' for commands.";
                    send(new_socket, welcome.c_str(), welcome.length(), 0);

                    std::string notification = "📢 " + default_nick + " has joined the room!";
                    broadcastMessage(notification.c_str(), new_socket);

                } else { // Message from existing client
                    char buffer[1024] = {0};
                    if (const int value_read = static_cast<int>(read(i, buffer, 1024)); value_read <= 0) {
                        // Client disconnected
                        std::cout << "❌ " << clients[i].nickname << " (" << clients[i].ip_address << ") has disconnected." << std::endl;
                        std::string notification = "📢 " + clients[i].nickname + " has left the room.";
                        broadcastMessage(notification.c_str(), i);

                        close(i);
                        FD_CLR(i, &master_set);
                        clients.erase(i);
                    } else {
                        // Message received
                        std::string message(buffer, value_read);
                        message.erase(message.find_last_not_of(" \n\r\t")+1); // Trim whitespace

                        if (!message.empty() && message[0] == '/') {
                            handleCommand(i, message);
                        } else if (!message.empty()) {
                            std::string prefix = clients[i].prefix + " [" + clients[i].nickname + "]: ";
                            std::string full_message = prefix + message;
                            broadcastMessage(full_message.c_str(), i);
                            std::cout << full_message << std::endl; // Show on host console
                        }
                    }
                }
            }
        }
    }
}

void Room::handleCommand(const int client_fd, const std::string& command) {
    if (clients[client_fd].permission_level >= 1 && command.rfind("/nick ", 0) == 0) {
        const std::string new_nickname = command.substr(6);
        if (new_nickname.empty() || new_nickname.length() > 20 || new_nickname.find(' ') != std::string::npos) {
            const auto msg = "❌ Invalid nickname. Must be 1-20 chars, no spaces.";
            send(client_fd, msg, strlen(msg), 0);
            return;
        }
        const std::string old_nickname = clients[client_fd].nickname;
        clients[client_fd].nickname = new_nickname;
        const std::string notification = "🔄 " + old_nickname + " is now known as " + new_nickname;
        broadcastMessage(notification.c_str(), -1);
        std::cout << notification << std::endl;

    } else if (command == "/users") {
        std::string user_list = "👥 Connected Users (" + std::to_string(clients.size()) + " total):\n";
        for (const auto &[nickname, ip_address, port, prefix, permission_level]: clients | std::views::values) {
            user_list += "  - " + prefix;
            user_list += nickname;
            user_list += " (" + ip_address + ":";
            user_list += port + ")";
            user_list += " [" + permission_level;
            user_list += "]\n";
        }
        send(client_fd, user_list.c_str(), user_list.length(), 0);

    } else if (command == "/help") {
        const auto msg = "📋 Commands:\n  /nick <name> - Change your nickname\n  /users - List connected users\n  /quit - Leave the room\n  /help - Show this message";
        send(client_fd, msg, strlen(msg), 0);

    } else {
        const auto msg = "❓ Unknown command. Type '/help' for a list.";
        send(client_fd, msg, strlen(msg), 0);
    }
}

void Room::broadcastMessage(const char* message, const int sender_fd) const {
    for (const auto &key: clients | std::views::keys) {
        if (key != sender_fd) {
            send(key, message, strlen(message), 0);
        }
    }
}

// --- Private Client Methods ---

void receiveMessages(const int sock) {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        if (const int value_read = static_cast<int>(read(sock, buffer, 1024)); value_read > 0) {
            std::cout << "\r" << buffer << std::endl;
            std::cout << "You: " << std::flush;
        } else {
             std::cout << "\n🔌 Server disconnected. Press Enter to exit." << std::endl;
             close(sock);
             exit(0);
        }
    }
}

void Room::clientLoop(int client_fd) {
    std::thread receiver_thread(receiveMessages, client_fd);
    receiver_thread.detach();

    std::string message;
    while (true) {
        std::cout << "You: " << std::flush;
        if (!std::getline(std::cin, message)) {
            break;
        }

        if (message == "/quit") {
            std::cout << "👋 Leaving the room. Goodbye!" << std::endl;
            break;
        }

        if (!message.empty()) {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }
    close(client_fd);
}

// --- Private Utility Methods ---

void Room::setupAddress(sockaddr_in& address, const int port, const std::string& ip) {
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (!ip.empty()) {
        if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
            std::cerr << "\nInvalid address/ Address not supported \n";
            exit(EXIT_FAILURE);
        }
    } else {
        address.sin_addr.s_addr = INADDR_ANY;
    }
}
