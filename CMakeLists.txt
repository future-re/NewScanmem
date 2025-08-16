# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(NewScanmem CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED COMPONENTS filesystem system)

# 1. 创建一个库来承载我们的模块
add_library(greeter_lib)

# 2. 将源文件添加到库中
target_sources(greeter_lib
        PUBLIC
        FILE_SET CXX_MODULES FILES
        src/value.cppm
        src/greeter.cppm
)

# 链接 Boost 到库
target_link_libraries(greeter_lib PUBLIC Boost::filesystem Boost::system)

# 3. 创建可执行文件
add_executable(NewScanmem src/main.cpp)

# 4. 将可执行文件链接到包含模块的库
target_link_libraries(NewScanmem PRIVATE greeter_lib)