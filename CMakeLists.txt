cmake_minimum_required(VERSION 3.23)

project(memseek VERSION 0.1.0 LANGUAGES CXX)

# 当使用新的查找/配置行为时，显式设置 CMP0167 为 NEW，强制使用包的 Config 模式
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

enable_testing()

# 设置 C++ 标准和要求
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json 以便 clangd 等工具使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 引入代码覆盖率支持
include(CodeCoverage)

# Prefer installed dependencies. FetchContent is only a fallback for a clean
# checkout, so normal offline builds do not unexpectedly access the network.
include(FetchContent)
# GoogleTest is needed only for tests.
find_package(GTest 1.14 CONFIG QUIET)
if(NOT TARGET GTest::gtest)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  )
  FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src)
add_subdirectory(test)

# 设置覆盖率报告（在所有子目录之后）
setup_coverage_reporting()
