#include "modification_framework.h"

auto main(int argc, char** argv) -> int {
    static volatile double marker = 12345.6789;
    constexpr double EXPECTEDVALUE = 12345.6789;
    constexpr double MODIFIEDVALUE = 99999.9999;
    constexpr double TOLERANCE = 0.001;

    // 使用通用框架运行测试
    return modification_framework::runModificationTest(
        argc, argv, marker, EXPECTEDVALUE, MODIFIEDVALUE, TOLERANCE);
}