include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
)

FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(parsley_tests ${TEST_SOURCES})

target_link_libraries(parsley_tests
    PRIVATE
        parsley
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(parsley_tests)
