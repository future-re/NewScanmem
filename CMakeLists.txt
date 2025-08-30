# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(NewScanmem CXX)

set(CMAKE_CXX_COMPILER "clang++-20")
set(CMAKE_C_COMPILER "clang-20")

# 设置 C++ 标准和要求
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_CLANG_TIDY "Run clang-tidy during build" ON)
if(ENABLE_CLANG_TIDY)
  if(EXISTS "${CMAKE_SOURCE_DIR}/scripts/clang-tidy-wrapper.sh")
    message(STATUS "Enabling clang-tidy via wrapper: ${CMAKE_SOURCE_DIR}/scripts/clang-tidy-wrapper.sh")
    set(CMAKE_CXX_CLANG_TIDY "${CMAKE_SOURCE_DIR}/scripts/clang-tidy-wrapper.sh")
  else()
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-20)
    if(CLANG_TIDY_EXE)
      message(STATUS "Enabling clang-tidy: ${CLANG_TIDY_EXE}")
      set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-p=${CMAKE_BINARY_DIR}")
    else()
      message(STATUS "clang-tidy not found, skipping CMAKE_CXX_CLANG_TIDY")
    endif()
  endif()
else()
  message(STATUS "clang-tidy disabled by ENABLE_CLANG_TIDY=OFF")
  set(CMAKE_CXX_CLANG_TIDY "")
endif()

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

add_subdirectory(src)
add_subdirectory(test)