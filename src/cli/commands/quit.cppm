/**
 * @file quit.cppm
 * @brief Quit command implementation
 */

module;

#include <expected>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.quit;

import cli.command;
import ui.show_message;

export namespace cli::commands {

/**
 * @class QuitCommand
 * @brief Exit the application
 */
class QuitCommand : public Command {
   public:
    [[nodiscard]] auto getName() const -> std::string_view override {
        return "quit";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Exit the application";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "quit";
    }

    [[nodiscard]] auto getAliases() const
        -> std::vector<std::string_view> override {
        return {"exit", "q"};
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (!args.empty()) {
            return std::unexpected("'quit' command takes no arguments");
        }

        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        (void)args;  // Unused

        ui::MessagePrinter::info("Goodbye!");

        return CommandResult{
            .success = true, .message = "", .shouldExit = true};
    }
};

}  // namespace cli::commands
