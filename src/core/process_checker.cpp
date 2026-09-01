#include "memseek/core/process_checker.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace core {
auto ProcessChecker::checkProcess(const pid_t pid) -> ProcessState {
    if (pid <= 0) return ProcessState::ERROR;
    const auto path =
        std::filesystem::path{"/proc"} / std::to_string(pid) / "status";
    if (!std::filesystem::exists(path)) return ProcessState::DEAD;
    std::ifstream status{path};
    if (!status) return ProcessState::ERROR;
    std::string line;
    while (std::getline(status, line)) {
        if (line.starts_with("State:"))
            return line.size() >= sizeof("State:\t")
                       ? parseProcessState(line[sizeof("State:\t") - 1])
                       : ProcessState::ERROR;
    }
    return ProcessState::ERROR;
}
auto ProcessChecker::isProcessDead(const pid_t pid) -> bool {
    return checkProcess(pid) != ProcessState::RUNNING;
}
auto ProcessChecker::parseProcessState(const char state) -> ProcessState {
    if (state < 'A' || state > 'Z') return ProcessState::ERROR;
    switch (state) {
        case 'Z':
        case 'X':
            return ProcessState::ZOMBIE;
        default:
            return ProcessState::RUNNING;
    }
}
}  // namespace core
