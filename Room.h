#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <netinet/in.h>

// Room.h
// This class manages the core chat room functionality.
// It can act as a server (for the room creator) or a client (for those joining).
class Room {
public:
    // Starts the chat room as a server on the specified port.
    void startServer(int port);

    // Joins an existing chat room at the given IP address and port.
    static void joinRoom(const std::string& ipAddress, int port);

private:
    // --- Server-specific methods ---
    // Handles incoming client connections and messages.
    void serverLoop(int server_fd);
    // Broadcasts a message to all connected clients except the sender.
    void broadcastMessage(const char* message, int sender_fd) const;

    // --- Client-specific methods ---
    // Handles sending and receiving messages for a client.
    static void clientLoop(int client_fd);

    // --- Utility methods ---
    // Sets up a socket address structure.
    static void setupAddress(struct sockaddr_in& address, int port, const std::string& ip = "");

    // A list of file descriptors for connected clients (server-only).
    std::vector<int> client_sockets;
};

#endif // ROOM_H
