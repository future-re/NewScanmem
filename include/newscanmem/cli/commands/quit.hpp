#pragma once

/**
 * @file quit.hpp
 * @brief Quit command implementation
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/cli/command.hpp"

namespace cli::commands {

/**
 * @class QuitCommand
 * @brief Exit the application
 */
class QuitCommand : public Command {
   public:
    [[nodiscard]] auto getName() const -> std::string_view override;

    [[nodiscard]] auto getDescription() const -> std::string_view override;

    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto getAliases() const
        -> std::vector<std::string_view> override;

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;
};

}  // namespace cli::commands
