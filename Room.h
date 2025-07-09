#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <netinet/in.h>
#include <map> // Required for std::map

// Room.h
// This class manages the core chat room functionality.
// It can act as a server (for the room creator) or a client (for those joining).

struct ClientInfo {
    std::string nickname;
    std::string ip_address;
    std::string port;
    std::string prefix;
    int permission_level; // ranges from 0-5 by default where 5 is host level permissions and 0 is guest permissions (no nicknames even).
};

class Room {
public:

    void startServer(int port);

    static void joinRoom(const std::string& ipAddress, int port);

private:
    // --- Server-specific methods ---

    void hostInputLoop(int server_fd);

    void serverLoop(int server_fd);

    void broadcastMessage(const char* message, int sender_fd) const;

    void handleCommand(int client_fd, const std::string& command);

    // --- Client-specific methods ---

    static void clientLoop(int client_fd);

    // --- Utility methods ---

    static void setupAddress(sockaddr_in& address, int port, const std::string& ip = "");

    // A map to store information about connected clients, keyed by socket descriptor.
    std::map<int, ClientInfo> clients;
};

#endif // ROOM_H
