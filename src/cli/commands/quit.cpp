#include "memseek/cli/commands/quit.hpp"

#include "memseek/ui/show_message.hpp"

namespace cli::commands {
auto QuitCommand::getName() const -> std::string_view { return "quit"; }
auto QuitCommand::getDescription() const -> std::string_view {
    return "Exit the application";
}
auto QuitCommand::getUsage() const -> std::string_view { return "quit"; }
auto QuitCommand::getAliases() const -> std::vector<std::string_view> {
    return {"exit", "q"};
}
auto QuitCommand::validateArgs(const std::vector<std::string>& arguments) const
    -> std::expected<void, std::string> {
    return arguments.empty()
               ? std::expected<void, std::string>{}
               : std::unexpected("'quit' command takes no arguments");
}
auto QuitCommand::execute(const std::vector<std::string>&)
    -> std::expected<CommandResult, std::string> {
    ui::MessagePrinter::info("Goodbye!");
    return CommandResult{.success = true, .message = {}, .shouldExit = true};
}
}  // namespace cli::commands
