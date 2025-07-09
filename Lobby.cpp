#include "Lobby.h"
#include "Room.h"
#include <iostream>
#include <limits>

// Lobby.cpp
// Implementation of the Lobby class.

void Lobby::show() {
    while (true) {
        std::cout << "\n===== Chat Lobby =====\n";
        std::cout << "1) Create a new room\n";
        std::cout << "2) Join an existing room\n";
        std::cout << "3) Exit\n";
        std::cout << "======================\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        // Input validation
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number.\n";
            continue;
        }
         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Consume newline

        switch (choice) {
            case 1: {
                int port;
                std::cout << "Enter the port number for your new room (e.g., 8080): ";
                std::cin >> port;
                if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Invalid port number.\n";
                    continue;
                }
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Consume newline
                std::cin.clear();
                Room room;
                room.startServer(port); // Start the room as a server
                break;
            }
            case 2: {
                std::string ipAddress;
                int port;
                std::cout << "Enter the IP address of the room: ";
                std::getline(std::cin, ipAddress);
                std::cout << "Enter the port number of the room: ";
                std::cin >> port;
                 if (std::cin.fail()) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Invalid port number.\n";
                    continue;
                }
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Consume newline
                std::cin.clear();
                Room::joinRoom(ipAddress, port); // Join a room as a client
                break;
            }
            case 3:
                std::cout << "Exiting lobby. Goodbye!\n";
                return;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }
}
