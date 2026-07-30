# --- Options -----------------------------------------------------------
set(supported_standards 20 23)
set(PARSLEY_TESTS_CXX_STANDARD 20 CACHE STRING "C++ standard used for building the tests.")

if(NOT PARSLEY_TESTS_CXX_STANDARD IN_LIST supported_standards)
    list(JOIN supported_standards ", " supported_standards_str)
    message(FATAL_ERROR "PARSLEY_TESTS_CXX_STANDARD must be one of: ${supported_standards_str}")
endif()

# --- Dependencies --------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(Catch2)

# --- Target ----------------------------------------------------------
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(parsley_tests ${TEST_SOURCES})

target_link_libraries(parsley_tests
    PRIVATE
        parsley
        Catch2::Catch2WithMain
)

set_target_properties(parsley_tests PROPERTIES
    CXX_STANDARD ${PARSLEY_TESTS_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# --- Test discovery ----------------------------------------------------
include(Catch)
catch_discover_tests(parsley_tests)

# --- Export Compile Commands --------------------------------------------
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    file(CREATE_LINK
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
        SYMBOLIC
    )
endif()
