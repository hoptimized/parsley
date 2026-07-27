cmake_minimum_required(VERSION 3.30)

project(
    parsley
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(parsley ${SOURCES})

target_include_directories(parsley
    PUBLIC
        include
    PRIVATE
        src)

option(PARSLEY_BUILD_TESTS "Build parsley tests" OFF)

if(PARSLEY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
