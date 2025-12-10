# Simple coverage helpers for GCC (lcov) and Clang (llvm-cov)

function(enable_coverage_for_targets)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  endif()

  # Determine compiler and set flags (apply at directory level BEFORE targets)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Coverage: using GCC flags and lcov")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Coverage: using Clang flags and llvm-cov")
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping -O0 -g)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    # Ensure a predictable output location for profraw when running tests
    set(LLVM_PROFILE_FILE "${CMAKE_BINARY_DIR}/default_%p.profraw" CACHE STRING "LLVM profile output" FORCE)
  else()
    message(WARNING "Coverage enabled, but unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()

# Adds a `coverage` custom target that runs tests and generates report into coverage/
function(add_coverage_target)
  set(COVERAGE_DIR ${CMAKE_SOURCE_DIR}/coverage)
  file(MAKE_DIRECTORY ${COVERAGE_DIR})

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_program(LCOV_EXEC lcov)
    find_program(GENHTML_EXEC genhtml)
    if(NOT LCOV_EXEC OR NOT GENHTML_EXEC)
      message(WARNING "lcov/genhtml not found; install to get HTML coverage (apt install lcov)")
    endif()

    add_custom_target(coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${LCOV_EXEC} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${COVERAGE_DIR}/coverage.info
      COMMAND ${LCOV_EXEC} --remove ${COVERAGE_DIR}/coverage.info '/usr/*' '${CMAKE_BINARY_DIR}/_deps/*' --output-file ${COVERAGE_DIR}/coverage.filtered.info
      COMMAND ${GENHTML_EXEC} -o ${COVERAGE_DIR}/html ${COVERAGE_DIR}/coverage.filtered.info
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running tests and generating lcov HTML report in coverage/html"
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Prefer LLVM 20 tools to match clang-20 generated profraw; fallback to unversioned
    find_program(LLVM_PROFDATA_EXEC NAMES llvm-profdata-20 llvm-profdata REQUIRED)
    find_program(LLVM_COV_EXEC NAMES llvm-cov-20 llvm-cov REQUIRED)
    message(STATUS "Coverage tools: profdata=${LLVM_PROFDATA_EXEC} cov=${LLVM_COV_EXEC}")

    # Clang flow: run tests with a defined LLVM_PROFILE_FILE, merge all profraw, then show coverage
    # Collect test binaries (executables only, exclude .cmake/.txt files)
    file(GLOB ALL_TEST_FILES "${CMAKE_BINARY_DIR}/test/test_test_*")
    set(TEST_BINS "")
    foreach(FILE ${ALL_TEST_FILES})
      if(NOT FILE MATCHES "\\.(cmake|txt)$")
        list(APPEND TEST_BINS "${FILE}")
      endif()
    endforeach()
    set(MAIN_BIN "${CMAKE_BINARY_DIR}/src/NewScanmem")

    add_custom_target(coverage
          # Remove stale profraw to avoid mismatched data
          COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/default_*.profraw
          # Run tests with coverage runtime writing profraw files
          COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/default_%p.profraw ${CMAKE_CTEST_COMMAND} --output-on-failure
          # Merge all generated profraw into a single profdata
          COMMAND ${LLVM_PROFDATA_EXEC} merge -sparse ${CMAKE_BINARY_DIR}/default_*.profraw -o ${COVERAGE_DIR}/coverage.profdata
          # Produce a concise summary report showing source code coverage (not test code)
          COMMAND ${LLVM_COV_EXEC} report -instr-profile=${COVERAGE_DIR}/coverage.profdata -ignore-filename-regex="test/|/usr/|_deps/" ${TEST_BINS} > ${COVERAGE_DIR}/summary.txt
          # Produce detailed line-level report for source code only
          COMMAND ${LLVM_COV_EXEC} show -instr-profile=${COVERAGE_DIR}/coverage.profdata -format=text -show-line-counts-or-regions -ignore-filename-regex="test/|/usr/|_deps/" ${TEST_BINS} > ${COVERAGE_DIR}/coverage.txt
          # Generate HTML report for easier browsing
          COMMAND ${LLVM_COV_EXEC} show -instr-profile=${COVERAGE_DIR}/coverage.profdata -format=html -show-line-counts-or-regions -ignore-filename-regex="test/|/usr/|_deps/" -output-dir=${COVERAGE_DIR}/html ${TEST_BINS}
      COMMENT "Running tests and generating llvm-cov reports (summary.txt, coverage.txt, html/)"
    )
  else()
    add_custom_target(coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMENT "Running tests; coverage tooling not available for compiler ${CMAKE_CXX_COMPILER_ID}"
    )
  endif()
endfunction()
