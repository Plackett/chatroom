# Specifies the minimum version of CMake required to build the project.
cmake_minimum_required(VERSION 3.31)

# Defines the project name and specifies that it's a C++ project.
project(chatroom CXX)

# Sets the C++ standard to be used. C++11 is required for threads.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Finds the standard Threads package, which is necessary for std::thread.
find_package(Threads REQUIRED)

# Creates the executable named 'chatroom' from the listed source files.
# IMPORTANT: All of your .cpp source files must be listed here.
add_executable(chatroom main.cpp Lobby.cpp Room.cpp)

# Links the threads library to your executable.
# This is crucial for the multi-threading parts of the chat application to work.
target_link_libraries(chatroom PRIVATE Threads::Threads)
