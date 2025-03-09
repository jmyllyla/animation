cmake_minimum_required(VERSION 3.10)
project(MyProject C)

set(CMAKE_C_STANDARD 99)

# Use FetchContent to download Raylib from GitHub
include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5  # You can change this to a specific tag or commit if needed
)
FetchContent_MakeAvailable(raylib)
FetchContent_Declare(
    logc
    GIT_REPOSITORY https://github.com/rxi/log.c.git
    GIT_TAG f9ea34994bd58ed342d2245cd4110bb5c6790153  
)
FetchContent_MakeAvailable(logc)
add_library(log STATIC ${logc_SOURCE_DIR}/src/log.c)

# Create an executable
add_executable(animation main.c)

target_include_directories(animation PRIVATE ${logc_SOURCE_DIR}/src/)
target_include_directories(animation PRIVATE ${raylib_SOURCE_DIR}/src)

target_link_libraries(animation raylib)
