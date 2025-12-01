/**
 * @file scanmem.cppm
 * @brief Scanmem entry module - delegates to CLI application framework
 */

module;

#include <expected>
#include <string>

export module scanmem;

import cli.app_config;
import cli.app;

export namespace scanmem {

/**
 * @brief Run scanmem with given configuration
 * @param config Application configuration
 * @return Exit code or error message
 */
[[nodiscard]] inline auto run(const cli::AppConfig& config)
    -> std::expected<int, std::string> {
    try {
        cli::Application app{config};
        return app.run();
    } catch (const std::exception& e) {
        return std::unexpected(std::string{"Runtime error: "} + e.what());
    }
}

}  // namespace scanmem
