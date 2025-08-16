// greeter.cppm
module;  // 全局模块分段
#include <iostream>
#include <string>

export module greeter;  // 声明模块接口

// 这是一个模块私有的辅助函数，因为它没有被 export
void print_prefix() { std::cout << "[LOG] "; }

// 这是一个导出的、公共的函数
export void say_hello(const std::string &name) {
    print_prefix();  // 可以调用模块内的私有函数
    std::cout << "Hello, " << name << "! Welcome to C++20 Modules."
              << std::endl;
}