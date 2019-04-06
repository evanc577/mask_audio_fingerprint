cmake_minimum_required(VERSION 3.14)

project(MASK)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(ENABLE_DFT ON CACHE INTERNAL "")

add_subdirectory(kfr)

set(SOURCES rtaudio/RtAudio.cpp)

add_executable(test main.cpp fingerprint.cpp ${SOURCES})

target_link_libraries(test kfr)
target_link_libraries(test kfr_dft)
include_directories(rtaudio)

set(CMAKE_CXX_LINK_FLAGS "-lpthread -lpulse-simple -lpulse")
add_definitions(-D__LINUX_PULSE__)
