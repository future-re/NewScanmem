#include <unistd.h>

#include <iomanip>
#include <iostream>

auto main() -> int {
    double value = 12345.6789;
    std::cout << "Initial value: " << value << std::endl;
    // 打印当前进程的pid
    std::cout << "Process ID: " << getpid() << std::endl;
    while (true) {
        std::cout << "Current value: " << value << " Cureent Address:" << &value
                  << std::endl;
        sleep(1);
    }
}