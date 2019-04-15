cmake_minimum_required(VERSION 3.14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
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

set(SOURCES rtaudio/RtAudio.cpp)

add_executable(ingest ingest.cpp audio_helper.cpp database.cpp fingerprint.cpp ${SOURCES})

include_directories(rtaudio)
include_directories(unqlite)

target_link_libraries(ingest kfr)
target_link_libraries(ingest kfr_dft)
target_link_libraries(ingest pulse-simple pulse)
target_link_libraries(ingest pthread)
target_link_libraries(ingest avcodec avutil avformat swresample)
target_link_libraries(ingest unqlite)

add_definitions(-D__LINUX_PULSE__)
