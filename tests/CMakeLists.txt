# Tests are picked up when new .cpp files are added, while CONFIGURE_DEPENDS
# makes the build system reconfigure automatically for those additions.
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# The integration and end-to-end suites have their own build/run flow.
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/integration/.*")
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/e2e/.*")

if(NOT TEST_SOURCES)
    message(STATUS "No unit test sources found")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt")
        add_subdirectory(integration)
    endif()
    return()
endif()

include(FetchContent)

# Prefer an installed GoogleTest. FetchContent is only a fallback for a clean
# checkout where GoogleTest is not available locally.
find_package(GTest 1.14 CONFIG QUIET)
if(NOT TARGET GTest::gtest_main)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

foreach(TEST_SRC ${TEST_SOURCES})
    file(RELATIVE_PATH TEST_REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${TEST_SRC}")
    get_filename_component(TEST_NAME "${TEST_SRC}" NAME_WE)
    string(REPLACE "/" "_" TEST_TARGET_SUFFIX "${TEST_REL_PATH}")
    string(REPLACE ".cpp" "" TEST_TARGET_SUFFIX "${TEST_TARGET_SUFFIX}")
    set(TEST_TARGET "test_${TEST_TARGET_SUFFIX}")

    add_executable(${TEST_TARGET} ${TEST_SRC})
    target_link_libraries(${TEST_TARGET} PRIVATE memseek_lib GTest::gtest_main)

    gtest_discover_tests(${TEST_TARGET}
        DISCOVERY_MODE POST_BUILD
        PROPERTIES
            DISCOVERED_TEST_PREFIX "${TEST_NAME}::"
            LABELS "unit"
    )
endforeach()

# Keep integration tests optional while that directory is absent or empty.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt")
    add_subdirectory(integration)
endif()
