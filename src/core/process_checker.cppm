/**
 * @file process_checker.cppm
 * @brief Process state checking via /proc filesystem (进程状态检查)
 */

module;

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

export module core.process_checker;

export namespace core {

/**
 * @enum ProcessState
 * @brief Process lifecycle states (进程生命周期状态)
 *
 * - RUNNING: Process is active (running/sleeping/waiting)
 * - ERROR: Invalid PID or check failure
 * - DEAD: Process no longer exists
 * - ZOMBIE: Process terminated but entry still in process table
 */
enum class ProcessState { RUNNING, ERROR, DEAD, ZOMBIE };

/**
 * @class ProcessChecker
 * @brief Utility for checking process state via /proc filesystem
 */
class ProcessChecker {
   public:
    /**
     * @brief Check process state by reading /proc/<pid>/status
     * @param pid Target process ID
     * @return ProcessState enum value
     */
    static auto checkProcess(pid_t pid) -> ProcessState {
        if (pid <= 0) {
            return ProcessState::ERROR;
        }

        const std::filesystem::path PROC_PATH =
            std::filesystem::path("/proc") / std::to_string(pid) / "status";

        if (!std::filesystem::exists(PROC_PATH)) {
            return ProcessState::DEAD;
        }

        std::ifstream statusFile(PROC_PATH);
        if (!statusFile.is_open()) {
            return ProcessState::ERROR;
        }

        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.starts_with("State:")) {
                if (line.size() >= sizeof("State:\t")) {
                    char state = line[sizeof("State:\t") - 1];
                    return parseProcessState(state);
                }
                break;
            }
        }

        return ProcessState::ERROR;
    }

    /**
     * @brief Check if process is no longer running
     * @param pid Target process ID
     * @return True if process is not in RUNNING state
     */
    static auto isProcessDead(pid_t pid) -> bool {
        return checkProcess(pid) != ProcessState::RUNNING;
    }

   private:
    /**
     * @brief Parse /proc/status State field character
     * @param state Single character from "State:" line (R/S/D/T/Z/X)
     * @return Corresponding ProcessState
     */
    static auto parseProcessState(char state) -> ProcessState {
        if (state < 'A' || state > 'Z') {
            return ProcessState::ERROR;
        }

        switch (state) {
            case 'Z':  // Zombie
            case 'X':  // Dead
                return ProcessState::ZOMBIE;
            case 'R':  // Running
            case 'S':  // Sleeping
            case 'D':  // Waiting
            case 'T':  // Stopped
            case 't':  // Tracing stop (though should be uppercase)
                return ProcessState::RUNNING;
            default:
                return ProcessState::RUNNING;
        }
    }
};

}  // namespace core