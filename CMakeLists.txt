cmake_minimum_required(VERSION 3.10)
project(AsciiAmp)

# 1. Standard C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Tell CMake where to find the "Address Book" for your libraries
# This allows find_package to work properly
# find_package(glfw3 REQUIRED)

# 3. Define your executable (The program you are building)
# Make sure "src/main.cpp" matches your actual file path!
add_executable(AsciiAmp 
    src/main.cpp
    libs/kissfft/kiss_fft.c
)


target_compile_definitions(AsciiAmp PRIVATE KISS_FFT_USE_FLOAT=1)
target_compile_definitions(AsciiAmp PRIVATE TAGLIB_STATIC)
target_include_directories(AsciiAmp PRIVATE 
    include    
    libs/termviz
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/taglib/dist/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/stb",
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/minimp3",
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/kissfft",
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/miniaudio/"
)

find_library(TAGLIB_PATH 
             NAMES tag libtag tag.dll  # Common MinGW search names
             PATHS "${CMAKE_CURRENT_SOURCE_DIR}/libs/taglib/dist/lib" 
             NO_DEFAULT_PATH
             )
if(NOT TAGLIB_PATH)
    message(FATAL_ERROR "Could not find TagLib! Check your dist/lib folder.")
endif()

# 4. The "Glue" - This connects the library to your program
# This handles BOTH the headers (.h) and the library (.lib)

# Link the actual file path found by the 'scout'
target_link_libraries(AsciiAmp PRIVATE ${TAGLIB_PATH})