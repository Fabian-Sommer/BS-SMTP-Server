cmake_minimum_required(VERSION 3.0)
project(SMTP-Server)

include_directories(netLink/include)
link_directories(netLink/out)
add_definitions(-O3 -std=c++1y)
add_executable(${PROJECT_NAME} Server.cpp)
target_link_libraries(${PROJECT_NAME} NetLink)
