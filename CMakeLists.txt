# CMakeLists.txt

cmake_minimum_required(VERSION 3.12.0)

if(MSVC)
    # 禁用 C4828 警告
    add_compile_options(/wd4828)
endif()

project(hdrd)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/qlementine/cmake")

# Set C++ standard 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add subdirectory for extern/qlementine
add_subdirectory(extern/qlementine)

add_subdirectory(app)
