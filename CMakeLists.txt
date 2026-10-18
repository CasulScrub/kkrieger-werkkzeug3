# Basic CMake build file for Linux (after removing Windows-specific files)
cmake_minimum_required(VERSION 3.10)
project(kkrieger_werkkzeug3)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Linux build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(SDL2 QUIET)

# Main executable target
add_executable(kkrieger_werkkzeug3
    # Source files from werkkzeug3_kkrieger (Windows-specific files removed)
    werkkzeug3_kkrieger/main.cpp
    werkkzeug3_kkrieger/werkwerkzeug.cpp
    werkkzeug3_kkrieger/werkwerkzeug.hpp
    werkkzeug3_kkrieger/main.hpp
    # Additional source files would be added here
)

# Link libraries (Linux equivalents for Windows libraries)
target_link_libraries(kkrieger_werkkzeug3
    SDL2::SDL2  # Linux equivalent for some graphics/audio needs
)

# Install target
install(TARGETS kkrieger_werkkzeug3 DESTINATION bin)
