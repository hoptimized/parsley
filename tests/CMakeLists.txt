# --- Options -----------------------------------------------------------
set(supported_standards 11 14 17 20 23)
set(PARSLEY_TESTS_CXX_STANDARD 17 CACHE STRING "C++ standard used for building the tests.")

if(NOT PARSLEY_TESTS_CXX_STANDARD IN_LIST supported_standards)
    list(JOIN supported_standards ", " supported_standards_str)
    message(FATAL_ERROR "PARSLEY_TESTS_CXX_STANDARD must be one of: ${supported_standards_str}")
endif()

# --- Dependencies --------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.3
)

FetchContent_MakeAvailable(doctest)

# --- Target ----------------------------------------------------------
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(parsley_tests ${TEST_SOURCES})

target_link_libraries(parsley_tests
    PRIVATE
        parsley
        doctest::doctest_with_main
)

set_target_properties(parsley_tests PROPERTIES
    CXX_STANDARD ${PARSLEY_TESTS_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# --- Test discovery ----------------------------------------------------
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(parsley_tests)

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
    # deps in _deps/ (e.g. doctest), it can guess a vendor file instead of our own
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
