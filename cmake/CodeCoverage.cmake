# CodeCoverage.cmake - LLVM Coverage support for C++20 modules
# 
# This module provides coverage analysis using LLVM tools (llvm-cov, llvm-profdata)
# which have excellent support for C++20 modules.
#
# Usage:
#   include(CodeCoverage)
#   add_coverage_target(TARGET_NAME executable_or_test)
#   setup_coverage_reporting()

option(ENABLE_COVERAGE "Enable code coverage analysis" OFF)

if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(WARNING "Coverage is best supported with Clang compiler")
    endif()

    # Find required tools
    find_program(LLVM_COV_PATH llvm-cov)
    find_program(LLVM_PROFDATA_PATH llvm-profdata)
    
    if(NOT LLVM_COV_PATH)
        message(FATAL_ERROR "llvm-cov not found! Please install LLVM tools.")
    endif()
    
    if(NOT LLVM_PROFDATA_PATH)
        message(FATAL_ERROR "llvm-profdata not found! Please install LLVM tools.")
    endif()
    
    message(STATUS "Code coverage enabled")
    message(STATUS "  llvm-cov: ${LLVM_COV_PATH}")
    message(STATUS "  llvm-profdata: ${LLVM_PROFDATA_PATH}")
    
    # Coverage compiler flags for Clang
    set(COVERAGE_COMPILER_FLAGS 
        -fprofile-instr-generate 
        -fcoverage-mapping
    )
    
    # Coverage linker flags
    set(COVERAGE_LINKER_FLAGS 
        -fprofile-instr-generate
    )
    
    # Add flags to compiler and linker
    add_compile_options(${COVERAGE_COMPILER_FLAGS})
    add_link_options(${COVERAGE_LINKER_FLAGS})
    
    # Set environment variable for profile output
    set(ENV{LLVM_PROFILE_FILE} "default.profraw")
endif()

# Function to add coverage target for a specific executable/test
function(add_coverage_target TARGET_NAME)
    if(ENABLE_COVERAGE)
        set_target_properties(${TARGET_NAME} PROPERTIES
            COMPILE_FLAGS "${COVERAGE_COMPILER_FLAGS}"
            LINK_FLAGS "${COVERAGE_LINKER_FLAGS}"
        )
    endif()
endfunction()

# Function to setup coverage reporting targets
function(setup_coverage_reporting)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()
    
    # Get all test executables
    set(TEST_EXECUTABLES "")
    get_property(all_tests DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY TESTS)
    
    # Create coverage report directory
    set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
    file(MAKE_DIRECTORY ${COVERAGE_OUTPUT_DIR})
    
    # Add custom target to merge profile data
    add_custom_target(coverage-merge
        COMMAND ${CMAKE_COMMAND} -E echo "Merging coverage data..."
        COMMAND ${LLVM_PROFDATA_PATH} merge -sparse 
            ${CMAKE_BINARY_DIR}/test/*.profraw
            -o ${COVERAGE_OUTPUT_DIR}/coverage.profdata
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Merging raw coverage data into coverage.profdata"
    )
    
    # Add custom target to generate HTML report
    add_custom_target(coverage-html
        COMMAND ${CMAKE_COMMAND} -E echo "Generating HTML coverage report..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}/html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating HTML coverage report"
        DEPENDS coverage-merge
    )
    
    # Add custom target to generate text summary
    add_custom_target(coverage-summary
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage Summary:"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Displaying coverage summary"
        DEPENDS coverage-merge
    )
    
    # Add custom target to generate lcov format (for IDE integration)
    add_custom_target(coverage-lcov
        COMMAND ${CMAKE_COMMAND} -E echo "Generating LCOV format..."
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating LCOV coverage data"
        DEPENDS coverage-merge
    )
    
    # Main coverage target that runs all tests and generates reports
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests with coverage..."
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target coverage-merge
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target coverage-html
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target coverage-summary
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in ${COVERAGE_OUTPUT_DIR}/html/index.html"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running complete coverage analysis"
    )
    
    # Add coverage check target with thresholds
    add_custom_target(coverage-check
        COMMAND ${CMAKE_COMMAND} -E echo "Checking coverage thresholds..."
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Verifying coverage meets minimum thresholds"
        DEPENDS coverage-merge
    )
    
endfunction()

# Function to generate coverage report for specific targets
function(generate_coverage_report)
    cmake_parse_arguments(COV "" "NAME;FORMAT" "TARGETS;SOURCES" ${ARGN})
    
    if(NOT ENABLE_COVERAGE)
        return()
    endif()
    
    if(NOT COV_NAME)
        set(COV_NAME "coverage")
    endif()
    
    if(NOT COV_FORMAT)
        set(COV_FORMAT "html")
    endif()
    
    set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(PROFDATA_FILE "${COVERAGE_OUTPUT_DIR}/coverage.profdata")
    
    # Build object list for llvm-cov
    set(OBJECT_ARGS "")
    foreach(TARGET ${COV_TARGETS})
        list(APPEND OBJECT_ARGS -object $<TARGET_FILE:${TARGET}>)
    endforeach()
    
    # Build source filter list
    set(SOURCE_ARGS "")
    if(COV_SOURCES)
        foreach(SRC ${COV_SOURCES})
            list(APPEND SOURCE_ARGS ${CMAKE_SOURCE_DIR}/${SRC})
        endforeach()
    endif()
    
    if(COV_FORMAT STREQUAL "html")
        add_custom_command(TARGET coverage-html POST_BUILD
            COMMAND ${LLVM_COV_PATH} show
                -instr-profile=${PROFDATA_FILE}
                ${OBJECT_ARGS}
                -format=html
                -output-dir=${COVERAGE_OUTPUT_DIR}/html
                -show-line-counts-or-regions
                -show-instantiation-summary
                ${SOURCE_ARGS}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating HTML report for ${COV_NAME}"
        )
    elseif(COV_FORMAT STREQUAL "text")
        add_custom_command(TARGET coverage-summary POST_BUILD
            COMMAND ${LLVM_COV_PATH} report
                -instr-profile=${PROFDATA_FILE}
                ${OBJECT_ARGS}
                ${SOURCE_ARGS}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating text summary for ${COV_NAME}"
        )
    elseif(COV_FORMAT STREQUAL "lcov")
        add_custom_command(TARGET coverage-lcov POST_BUILD
            COMMAND ${LLVM_COV_PATH} export
                -instr-profile=${PROFDATA_FILE}
                ${OBJECT_ARGS}
                -format=lcov
                > ${COVERAGE_OUTPUT_DIR}/${COV_NAME}.lcov
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating LCOV format for ${COV_NAME}"
        )
    endif()
    
endfunction()
