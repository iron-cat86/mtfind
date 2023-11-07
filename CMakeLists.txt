cmake_minimum_required(VERSION 3.26)
project(mtfind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(${PROJECT_NAME} "main.cpp")
#target_include_directories(mtfind PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mtfind PUBLIC pthread)
