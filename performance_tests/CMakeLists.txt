cmake_minimum_required(VERSION 3.10)

# Project name
project(DosatsuPerformanceTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(std_library_performance_test std_library_performance_test.cpp)

# Set compiler flags for consistency with main project
if(MSVC)
    target_compile_options(std_library_performance_test PRIVATE /W4)
else()
    target_compile_options(std_library_performance_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

