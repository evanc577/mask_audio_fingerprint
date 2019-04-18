cmake_minimum_required(VERSION 3.14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(MASK)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(ENABLE_DFT ON CACHE INTERNAL "")

add_subdirectory(kfr)
add_subdirectory(unqlite)
include_directories(rtaudio)

link_libraries(kfr kfr_dft unqlite pthread)
add_definitions(-D__LINUX_PULSE__)

add_executable(ingest ingest.cpp audio_helper.cpp database.cpp fingerprint.cpp)
target_link_libraries(ingest avcodec avutil avformat swresample)

add_executable(identify identify.cpp database.cpp fingerprint.cpp rtaudio/RtAudio.cpp)
target_link_libraries(identify pulse-simple pulse)
