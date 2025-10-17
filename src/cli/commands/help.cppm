/**
 * @file help.cppm
 * @brief Help command implementation
 */

module;

#include <algorithm>
#include <expected>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.help;

import cli.command;
import ui.show_message;

export namespace cli::commands {

/**
 * @class HelpCommand
 * @brief Display help information for all commands or a specific command
 */
class HelpCommand : public Command {
   public:
    [[nodiscard]] auto getName() const -> std::string_view override {
        return "help";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Display help information about available commands";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "help [command_name]";
    }

    [[nodiscard]] auto getAliases() const
        -> std::vector<std::string_view> override {
        return {"?", "h"};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        auto& registry = CommandRegistry::instance();

        // If a specific command is requested
        if (!args.empty()) {
            return showCommandHelp(args[0], registry);
        }

        // Show general help
        return showGeneralHelp(registry);
    }

   private:
    static auto showCommandHelp(const std::string& commandName,
                                CommandRegistry& registry)
        -> std::expected<CommandResult, std::string> {
        auto* cmd = registry.findCommand(commandName);
        if (cmd == nullptr) {
            return std::unexpected("Unknown command: " + commandName);
        }

        std::ostringstream oss;
        oss << "\nCommand: " << cmd->getName() << "\n";
        oss << "Description: " << cmd->getDescription() << "\n";
        oss << "Usage: " << cmd->getUsage() << "\n";

        auto aliases = cmd->getAliases();
        if (!aliases.empty()) {
            oss << "Aliases: ";
            for (size_t i = 0; i < aliases.size(); ++i) {
                if (i > 0) {
                    oss << ", ";
                }
                oss << aliases[i];
            }
            oss << "\n";
        }

        ui::MessagePrinter::info(oss.str());
        return CommandResult{.success = true, .message = ""};
    }

    static auto showGeneralHelp(CommandRegistry& registry)
        -> std::expected<CommandResult, std::string> {
        auto commands = registry.getAllCommands();
        if (commands.empty()) {
            ui::MessagePrinter::warn("No commands registered");
            return CommandResult{.success = true, .message = ""};
        }

        // Sort commands by name
        std::ranges::sort(commands, {}, &Command::getName);

        std::ostringstream oss;
        oss << "\n=== Available Commands ===\n\n";

        // Find maximum command name length for alignment
        size_t maxNameLen = 0;
        for (const auto* cmd : commands) {
            maxNameLen = std::max(maxNameLen, cmd->getName().length());
        }

        for (const auto* cmd : commands) {
            oss << "  " << cmd->getName();

            // Padding for alignment
            for (size_t i = cmd->getName().length(); i < maxNameLen + 2; ++i) {
                oss << " ";
            }

            oss << cmd->getDescription() << "\n";
        }

        oss << "\nType 'help <command>' for more information about a specific "
               "command.\n";

        ui::MessagePrinter::info(oss.str());
        return CommandResult{.success = true, .message = ""};
    }
};

}  // namespace cli::commands
