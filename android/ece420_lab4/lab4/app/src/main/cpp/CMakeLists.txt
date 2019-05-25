cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -Wall -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O3 -g")

set(ENABLE_DFT ON CACHE INTERNAL "")
set(CPU_ARCH "generic" CACHE INTERNAL "")
set(ANDROID ON CACHE INTERNAL "")

add_subdirectory(unqlite)
add_subdirectory(kfr)

add_library(echo SHARED
            audio_common.cpp
            audio_main.cpp
            audio_player.cpp
            audio_recorder.cpp
            debug_utils.cpp
            kiss_fft/kiss_fft.c
            unqlite/unqlite.c
            ece420_lib.cpp
            ece420_main.cpp
            fingerprint.cpp
            database.cpp
        )

# include libraries needed for hello-jni lib
target_link_libraries(echo
                      android
                      atomic
                      log
                      OpenSLES
                      unqlite
                      kfr
                      kfr_dft)

# Android Studio 2.2.0 with CMake support does not pack stl shared libraries,
# so app needs to pack the right shared lib into APK. This sample uses solution
# from https://github.com/jomof/ndk-stl to find the right stl shared lib to use
# and copy it to the right place for Android Studio to pack
# Usage: download ndk-stl-config.cmake into app's directory hosting CMakeLists.txt
#        and just use it with the following line
include(ndk-stl-config.cmake)
