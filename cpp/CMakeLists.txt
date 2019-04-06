cmake_minimum_required(VERSION 3.1)

project(MASK)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS}" CACHE STRING "compile flags" FORCE)
set(CMAKE_CXX_LINK_FLAGS "-lpthread -lpulse-simple -lpulse")

include_directories(rtaudio)
add_subdirectory(kfr)

set(SOURCES rtaudio/RtAudio.cpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-D__LINUX_PULSE__)

add_executable(test main.cpp fingerprint.cpp rtaudio/RtAudio.cpp)

target_link_libraries(test kfr)
target_link_libraries(test kfr_dft)
target_sources(test PRIVATE ${KFR_DFT_SRC})
