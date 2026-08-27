#include "newscanmem/cli/commands/help.hpp"

#include <algorithm>
#include <sstream>

#include "newscanmem/ui/show_message.hpp"

namespace cli::commands {

auto HelpCommand::getName() const -> std::string_view { return "help"; }

auto HelpCommand::getDescription() const -> std::string_view {
    return "Display help information about available commands";
}

auto HelpCommand::getUsage() const -> std::string_view {
    return "help [command_name]";
}

auto HelpCommand::getAliases() const -> std::vector<std::string_view> {
    return {"?", "h"};
}

auto HelpCommand::execute(const std::vector<std::string>& args)
    -> std::expected<CommandResult, std::string> {
    auto& registry = CommandRegistry::instance();
    return args.empty() ? showGeneralHelp(registry)
                        : showCommandHelp(args[0], registry);
}

auto HelpCommand::showCommandHelp(const std::string& commandName,
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
            if (i > 0) oss << ", ";
            oss << aliases[i];
        }
        oss << "\n";
    }
    ui::MessagePrinter::info(oss.str());
    return CommandResult{.success = true, .message = ""};
}

auto HelpCommand::showGeneralHelp(CommandRegistry& registry)
    -> std::expected<CommandResult, std::string> {
    auto commands = registry.getAllCommands();
    if (commands.empty()) {
        ui::MessagePrinter::warn("No commands registered");
        return CommandResult{.success = true, .message = ""};
    }
    std::ranges::sort(commands, {}, &Command::getName);
    std::ostringstream oss;
    oss << "\n=== Available Commands ===\n\n";
    size_t maxNameLen = 0;
    for (const auto* cmd : commands)
        maxNameLen = std::max(maxNameLen, cmd->getName().length());
    for (const auto* cmd : commands) {
        oss << "  " << cmd->getName();
        for (size_t i = cmd->getName().length(); i < maxNameLen + 2; ++i)
            oss << " ";
        oss << cmd->getDescription() << "\n";
    }
    oss << "\nType 'help <command>' for more info.\n";
    oss << "\nScan Types: "
           "int|int8|i8|int16|i16|int32|i32|int64|i64|float|double|any|"
           "anyint|anyfloat|string|str|bytearray|bytes\n";
    oss << "Match Types: any, =|eq, !=|neq, gt|>, lt|<, range, changed, "
           "notchanged|update, inc|increased, dec|decreased, incby, decby\n";
    oss << "Examples:\n";
    oss << "  scan int64 any               (初次快照)\n";
    oss << "  scan int64 = 123             (按值筛选)\n";
    oss << "  scan int changed             (自动基线+变化过滤)\n";
    oss << "  scan int range 100 200       (范围过滤)\n";
    oss << "  scan int64 incby 4           (按差值过滤)\n";
    oss << "\nAuto baseline: 首次使用依赖旧值的匹配 "
           "(changed/inc/dec/incby/decby/notchanged) 时自动做一次 'any' "
           "快照再过滤。\n";
    ui::MessagePrinter::info(oss.str());
    return CommandResult{.success = true, .message = ""};
}

}  // namespace cli::commands
