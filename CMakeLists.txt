# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

find_program(CLANGXX20 clang++-20)
find_program(CLANG20 clang-20)
if(CLANGXX20 AND CLANG20)
  set(CMAKE_CXX_COMPILER ${CLANGXX20} CACHE STRING "C++ compiler" FORCE)
  set(CMAKE_C_COMPILER ${CLANG20} CACHE STRING "C compiler" FORCE)
endif()

project(NewScanmem CXX)

# 设置 C++ 标准和要求
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json 以便 clangd 等工具使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(MODULE_CACHE_DIR "${CMAKE_BINARY_DIR}/clang-module-cache")
file(MAKE_DIRECTORY "${MODULE_CACHE_DIR}")

# 统一的 Clang 模块选项（INTERFACE，供所有目标继承）
add_library(clang_module_flags INTERFACE)
target_compile_options(clang_module_flags INTERFACE
  $<$<CXX_COMPILER_ID:Clang>:
    -fmodules -fcxx-modules -fimplicit-module-maps -fno-implicit-modules -fmodules-cache-path=${MODULE_CACHE_DIR}
  >
)

# 添加编译gtest库
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src)
add_subdirectory(test)