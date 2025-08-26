module;

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

export module process_checker;

export enum class ProcessState { 
    RUNNING,
    ERROR,
    DEAD,
    ZOMBIE
};

export class ProcessChecker {
   public:
    static ProcessState checkProcess(pid_t pid) {
        if (pid <= 0) {
            return ProcessState::ERROR;
        }

        const std::filesystem::path procPath =
            std::filesystem::path("/proc") / std::to_string(pid) / "status";

        if (!std::filesystem::exists(procPath)) {
            return ProcessState::DEAD;
        }

        std::ifstream statusFile(procPath);
        if (!statusFile.is_open()) {
            return ProcessState::ERROR;
        }

        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.rfind("State:", 0) == 0) {
                if (line.size() >= sizeof("State:\t")) {
                    char state = line[sizeof("State:\t") - 1];
                    return parseProcessState(state);
                }
                break;
            }
        }

        return ProcessState::ERROR;
    }

    static bool isProcessDead(pid_t pid) {
        return checkProcess(pid) != ProcessState::RUNNING;
    }

   private:
    static ProcessState parseProcessState(char state) {
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