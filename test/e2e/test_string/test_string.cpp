#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>

auto main() -> int {
    std::string str = "Hello, World!";
    std::cout << getpid() << std::endl;
    while (true) {
        std::cout << str << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}