cmake_minimum_required(VERSION 3.30)

project(
    parsley
    VERSION 0.1
    LANGUAGES CXX
)

add_library(parsley INTERFACE)
add_library(parsley::parsley ALIAS parsley)

target_include_directories(parsley INTERFACE include)

option(PARSLEY_BUILD_TESTS "Build parsley tests" OFF)

if(PARSLEY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
