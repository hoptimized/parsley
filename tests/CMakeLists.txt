# --- Options -----------------------------------------------------------
set(supported_standards 17 20 23)
set(PARSLEY_TESTS_CXX_STANDARD 17 CACHE STRING "C++ standard used for building the tests.")

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

    # Symlink compile database from the build directory to the project root
    # so that clangd can find it for code browsing.
    file(CREATE_LINK
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
        SYMBOLIC
    )

    # clangd guesses compile flags for headers (nothing compiles a .h directly)
    # by picking a similar-looking .cpp from compile_commands.json. With vendored
    # deps in _deps/ (e.g. Catch2), it can guess a vendor file instead of our own
    # test .cpp files, causing errors on code that actually compiles fine.
    # Fix: strip _deps/ entries so it can only match our own files.
    add_custom_target(prune-compile-commands ALL
        COMMAND jq [=[[.[] | select(.file | contains("_deps") | not)]]=]
                ${CMAKE_BINARY_DIR}/compile_commands.json > ${CMAKE_BINARY_DIR}/cc_pruned.json
        COMMAND ${CMAKE_COMMAND} -E rename
                ${CMAKE_BINARY_DIR}/cc_pruned.json ${CMAKE_BINARY_DIR}/compile_commands.json
        VERBATIM
    )
endif()
