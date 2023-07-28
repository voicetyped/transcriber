cmake_minimum_required(VERSION 3.1)
set(CMAKE_BUILD_TYPE Debug)
set(TARGET transcriber)
project(${TARGET})

# Add the source files for your C++ web server
set(SERVER_SOURCES main.cpp utilities.h dr_wav.h httplib.h whisper.cpp/whisper.h transcriber.cpp transcriber.h audio_tooling.cpp audio_tooling.h)

# Add any other necessary include directories or libraries
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(whisper.cpp)

find_package(Boost REQUIRED COMPONENTS system thread)
# Find FFmpeg package

include(FetchContent)


#libxid
FetchContent_Declare(libxid
        URL https://github.com/uditha-atukorala/libxid/archive/refs/tags/v0.1.0.tar.gz
        URL_HASH SHA256=31589bb5274c9d25a8b6c49ee04a6c76151f10082e7feb13314be02a4b2d58c8
        )
FetchContent_MakeAvailable(libxid)

# Create the executable for the C++ web server
add_executable(${TARGET} ${SERVER_SOURCES})
include(whisper.cpp/cmake/DefaultTargetOptions.cmake)

# Link Boost and pthread libraries
target_link_libraries(${TARGET} PRIVATE
        Boost::system Boost::thread pthread
        whisper libxid::xid)
# Optionally, set the output directory for the executable
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")