#include <unistd.h>

#include <iostream>
#include <string>

#include "modification_framework.h"

auto main(int argc, char** argv) -> int {
    static std::string marker = "Hello, World!";
    const std::string EXPECTEDVALUE = "Hello, World!";
    const std::string MODIFIEDVALUE = "Modified String!";
    return modification_framework::runModificationTest(
        argc, argv, marker, EXPECTEDVALUE, MODIFIEDVALUE);
}