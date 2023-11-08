cmake_minimum_required(VERSION 3.26)
project(mtfind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(${PROJECT_NAME} "main.cpp")

find_package(Threads REQUIRED)

target_link_libraries(mtfind PUBLIC Threads::Threads)
