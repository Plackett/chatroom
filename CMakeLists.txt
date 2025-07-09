cmake_minimum_required(VERSION 3.31)

project(chatroom CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(chatroom main.cpp Lobby.cpp Room.cpp)

target_link_libraries(chatroom PRIVATE Threads::Threads)
