/**
 * @file app_config.cppm
 * @brief Application configuration structure
 */

module;

#include <sys/types.h>

#include <optional>
#include <string>

export module cli.app_config;

export namespace cli {

struct AppConfig {
    pid_t targetPid{0};
    bool debugMode{false};
    bool exitOnError{false};
    std::optional<std::string> initialCommands;

    [[nodiscard]] auto isValid() const noexcept -> bool {
        return targetPid >= 0;
    }
};

}  // namespace cli
