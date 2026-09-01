#pragma once

/**
 * @file help.hpp
 * @brief Help command implementation
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "memseek/cli/command.hpp"

namespace cli::commands {

/**
 * @class HelpCommand
 * @brief Display help information for all commands or a specific command
 */
class HelpCommand : public Command {
   public:
    [[nodiscard]] auto getName() const -> std::string_view override;

    [[nodiscard]] auto getDescription() const -> std::string_view override;

    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto getAliases() const
        -> std::vector<std::string_view> override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;

   private:
    static auto showCommandHelp(const std::string& commandName,
                                CommandRegistry& registry)
        -> std::expected<CommandResult, std::string>;

    static auto showGeneralHelp(CommandRegistry& registry)
        -> std::expected<CommandResult, std::string>;
};

}  // namespace cli::commands
