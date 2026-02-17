cmake_minimum_required(VERSION 3.16)
project(MyProject C)

set(CMAKE_C_STANDARD 23)

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
)
FetchContent_MakeAvailable(logc)
add_library(log STATIC ${logc_SOURCE_DIR}/src/log.c)

# Create an executable
add_executable(animation main.c)

target_include_directories(animation PRIVATE ${logc_SOURCE_DIR}/src/)
target_include_directories(animation PRIVATE ${raylib_SOURCE_DIR}/src)

target_link_libraries(animation raylib)

if (EMSCRIPTEN)
    set_target_properties(animation PROPERTIES SUFFIX ".html")
    target_link_options(animation PRIVATE
        "SHELL:-sASYNCIFY"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/IMG_1.jpeg@/IMG_1.jpeg"
    )
endif()
