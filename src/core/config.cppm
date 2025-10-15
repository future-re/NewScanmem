/**
 * @file config.cppm
 * @brief Configuration management for scanmem
 */

module;

#include <unistd.h>

#include <expected>
#include <string>

export module core.config;

export namespace core {

/**
 * @struct ScanMemConfig
 * @brief Configuration structure for scanmem application
 */
struct ScanMemConfig {
    pid_t targetPid = 0;
    bool debugMode = false;
    bool exitOnError = false;
    bool backendMode = false;
    std::string historyFile;

    /**
     * @brief Check if configuration is valid
     * @return True if configuration is valid
     */
    [[nodiscard]] auto isValid() const noexcept -> bool {
        return targetPid > 0;
    }

    /**
     * @brief Validate configuration
     * @return Expected void or error message
     */
    [[nodiscard]] auto validate() const -> std::expected<void, std::string> {
        if (targetPid <= 0) {
            return std::unexpected("Invalid PID: must be positive");
        }
        return {};
    }
};

/**
 * @class ConfigManager
 * @brief Manages application configuration
 */
class ConfigManager {
   public:
    /**
     * @brief Get default configuration
     * @return Default configuration instance
     */
    [[nodiscard]] static auto getDefault() -> ScanMemConfig {
        return ScanMemConfig{};
    }

    /**
     * @brief Create configuration from command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return Expected configuration or error message
     */
    [[nodiscard]] static auto fromCommandLine(int argc, char* argv[])
        -> std::expected<ScanMemConfig, std::string> {
        ScanMemConfig config = getDefault();

        // Simple argument parsing (to be enhanced with proper parser)
        for (int idx = 1; idx < argc; ++idx) {
            std::string arg = argv[idx];

            if (arg == "--debug" || arg == "-d") {
                config.debugMode = true;
            } else if (arg == "--backend" || arg == "-b") {
                config.backendMode = true;
            } else if (arg == "--exit-on-error" || arg == "-e") {
                config.exitOnError = true;
            } else if (arg == "--pid" || arg == "-p") {
                if (idx + 1 < argc) {
                    try {
                        config.targetPid = std::stoi(argv[++idx]);
                    } catch (...) {
                        return std::unexpected("Invalid PID value");
                    }
                } else {
                    return std::unexpected("--pid requires a value");
                }
            } else if (arg.starts_with("--pid=")) {
                try {
                    config.targetPid = std::stoi(arg.substr(6));
                } catch (...) {
                    return std::unexpected("Invalid PID value");
                }
            }
        }

        return config;
    }

    /**
     * @brief Save configuration to file
     * @param cfg Configuration to save
     * @return Expected void or error message
     */
    static auto save(const ScanMemConfig& cfg)
        -> std::expected<void, std::string> {
        // TODO: Implement configuration file saving
        (void)cfg;
        return {};
    }

    /**
     * @brief Load configuration from file
     * @return Expected configuration or error message
     */
    static auto load() -> std::expected<ScanMemConfig, std::string> {
        // TODO: Implement configuration file loading
        return getDefault();
    }
};

}  // namespace core
