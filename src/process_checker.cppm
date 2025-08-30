module;

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

export module process_checker;

/**
 * @enum ProcessState
 * @brief 表示进程可能的状态。
 *
 * 枚举进程生命周期中的各种状态。
 * - RUNNING: 进程正在运行。
 * - ERROR: 进程发生错误。
 * - DEAD: 进程已终止且不再运行。
 * - ZOMBIE: 进程已结束但仍在进程表中有条目。
 */
export enum class ProcessState { RUNNING, ERROR, DEAD, ZOMBIE };

/**
 * @brief 检查指定进程ID（pid）的进程状态。
 *
 * 此静态函数根据传入的进程ID（pid）判断进程的状态，并返回相应的ProcessState枚举值。
 * 如果pid小于等于0，则返回ProcessState::ERROR，表示进程ID无效或发生错误。
 *
 * @param pid 要检查的进程ID。
 * @return ProcessState 进程的当前状态。
 */
export class ProcessChecker {
   public:
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

    static auto isProcessDead(pid_t pid) -> bool {
        return checkProcess(pid) != ProcessState::RUNNING;
    }

   private:
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