cmake_minimum_required(VERSION 3.28)

project(NewScanmem VERSION 0.1.0 LANGUAGES CXX)

enable_testing()

# 设置 C++ 标准和要求
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 CMake 的原生 C++ Modules 支持（避免与 Clang 的 header-modules 选项混用）
# 3.28 提供实验性 API；开启后由 CMake 负责扫描与 BMI 生成/复用
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "max")
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# 生成 compile_commands.json 以便 clangd 等工具使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 添加编译gtest库
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src)
add_subdirectory(test)

# 复制 compile_commands.json 到项目根目录，方便 clangd 自动检测
add_custom_target(copy_compile_commands ALL
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/copy_compile_commands.cmake
  BYPRODUCTS ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMENT "Copy compile_commands.json to project root for clangd")