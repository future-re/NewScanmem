#pragma once

#include <unistd.h>

namespace core {
enum class ProcessState { RUNNING, ERROR, DEAD, ZOMBIE };
class ProcessChecker {
   public:
    [[nodiscard]] static auto checkProcess(pid_t pid) -> ProcessState;
    [[nodiscard]] static auto isProcessDead(pid_t pid) -> bool;

   private:
    [[nodiscard]] static auto parseProcessState(char state) -> ProcessState;
};
}  // namespace core
