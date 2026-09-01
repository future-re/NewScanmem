#pragma once

/**
 * @file scanmem.hpp
 * @brief Scanmem entry module - delegates to CLI application framework
 */

#include <expected>
#include <string>

#include "memseek/cli/app_config.hpp"

namespace scanmem {

[[nodiscard]] auto run(const cli::AppConfig& config)
    -> std::expected<int, std::string>;

}  // namespace scanmem
