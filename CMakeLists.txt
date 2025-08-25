# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(NewScanmem CXX)

set(CMAKE_CXX_COMPILER "clang++-20")
set(CMAKE_C_COMPILER "clang-20")

# 设置 C++ 标准和要求
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json 以便 clangd 等工具使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(find_boost)

# 查找 GTest 库
find_package(GTest REQUIRED)

# --- 核心模块库 ---
# 使用 STATIC 库聚合并编译 C++ 模块（避免 INTERFACE 在某些版本上的内部错误）
add_library(NewScanmem_lib STATIC)

# 要求此库及其使用者支持 C++23（不使用 cxx_modules 特性标志）
target_compile_features(NewScanmem_lib PUBLIC cxx_std_23)

# 将模块源文件添加到库的 PUBLIC 文件集中
target_sources(NewScanmem_lib
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/show_message.cppm
        src/value.cppm
        src/targetmem.cppm
        src/sets.cppm
        src/process_checker.cppm
        src/endianness.cppm
        src/maps.cppm
)

# 链接 Boost 到库
target_link_libraries(NewScanmem_lib PUBLIC Boost::filesystem Boost::regex)


# --- 主可执行文件 ---
add_executable(NewScanmem src/main.cpp)
target_link_libraries(NewScanmem PRIVATE NewScanmem_lib)


# --- 测试 ---
enable_testing()

# Sets模块测试
add_executable(test_sets test/test_sets.cpp)
target_link_libraries(test_sets PRIVATE GTest::GTest GTest::Main NewScanmem_lib)

# ProcessChecker模块测试
add_executable(test_process_checker test/test_process_checker.cpp)
target_link_libraries(test_process_checker PRIVATE GTest::GTest GTest::Main NewScanmem_lib)

include(GoogleTest)
gtest_discover_tests(test_sets)
gtest_discover_tests(test_process_checker)