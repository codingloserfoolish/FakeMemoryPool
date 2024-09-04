cmake_minimum_required(VERSION 3.10)
project(MemoryPool)
set(CMAKE_CXX_STANDARD 11)
add_executable(${PROJECT_NAME} main.cpp "memoryPool.h" "memoryPool.cpp")