# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(MyModuleProject CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. 创建一个库来承载我们的模块
add_library(greeter_lib)

# 2. 将源文件添加到库中
target_sources(greeter_lib
        PUBLIC
        FILE_SET CXX_MODULES FILES
        greeter.cppm
)

# 3. 创建可执行文件
add_executable(my_app main.cpp)

# 4. 将可执行文件链接到包含模块的库
target_link_libraries(my_app PRIVATE greeter_lib)