# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

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
# add_subdirectory(test)

# 添加 MkDocs 构建和部署的自定义目标
option(BUILD_DOCS "Build MkDocs documentation as part of the default build" ON)

# MkDocs build target
if(BUILD_DOCS)
  add_custom_target(mkdocs_build ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building MkDocs documentation..."
    COMMAND mkdocs build --clean
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/mkdocs
    COMMENT "Building MkDocs documentation"
  )
else()
  add_custom_target(mkdocs_build
    COMMAND ${CMAKE_COMMAND} -E echo "Building MkDocs documentation..."
    COMMAND mkdocs build --clean
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/mkdocs
    COMMENT "Building MkDocs documentation"
  )
endif()

# MkDocs deploy target (manual trigger)
add_custom_target(mkdocs_deploy
  COMMAND ${CMAKE_COMMAND} -E echo "Deploying MkDocs documentation to GitHub Pages..."
  COMMAND mkdocs gh-deploy --force --config-file mkdocs/mkdocs.yml
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/mkdocs
  COMMENT "Deploying MkDocs documentation"
)