/**
 * @file command.cppm
 * @brief Command pattern base classes and registry for CLI
 */

module;

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module cli.command;

export namespace cli {

/**
 * @struct CommandResult
 * @brief Result of command execution
 */
struct CommandResult {
    bool success = true;
    std::string message;
    bool shouldExit = false;  // Signal to exit the REPL
};

/**
 * @class Command
 * @brief Abstract base class for all CLI commands
 */
class Command {
   public:
    virtual ~Command() = default;

    /**
     * @brief Get command name
     * @return Command name (e.g., "pid", "scan", "write")
     */
    [[nodiscard]] virtual auto getName() const -> std::string_view = 0;

    /**
     * @brief Get command description
     * @return Short description of what the command does
     */
    [[nodiscard]] virtual auto getDescription() const -> std::string_view = 0;

    /**
     * @brief Get command usage information
     * @return Usage string (e.g., "pid <process_id>")
     */
    [[nodiscard]] virtual auto getUsage() const -> std::string_view = 0;

    /**
     * @brief Get command aliases
     * @return Vector of alternative names for this command
     */
    [[nodiscard]] virtual auto getAliases() const
        -> std::vector<std::string_view> {
        return {};
    }

    /**
     * @brief Execute the command
     * @param args Command arguments (not including command name)
     * @return Expected result or error message
     */
    [[nodiscard]] virtual auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> = 0;

    /**
     * @brief Validate command arguments
     * @param args Command arguments
     * @return True if arguments are valid
     */
    [[nodiscard]] virtual auto validateArgs(
        const std::vector<std::string>& args) const
        -> std::expected<void, std::string> {
        // Default: no validation
        (void)args;
        return {};
    }
};

/**
 * @class CommandRegistry
 * @brief Registry for managing available commands
 */
class CommandRegistry {
   public:
    /**
     * @brief Get singleton instance
     * @return Reference to the command registry
     */
    static auto instance() -> CommandRegistry& {
        static CommandRegistry registry;
        return registry;
    }

    /**
     * @brief Register a command
     * @param command Unique pointer to command
     */
    auto registerCommand(std::unique_ptr<Command> command) -> void {
        if (!command) {
            return;
        }

        auto name = std::string(command->getName());

        // Register aliases
        for (const auto& alias : command->getAliases()) {
            aliases_[std::string(alias)] = name;
        }

        commands_[name] = std::move(command);
    }

    /**
     * @brief Find command by name or alias
     * @param name Command name or alias
     * @return Pointer to command, or nullptr if not found
     */
    [[nodiscard]] auto findCommand(std::string_view name) -> Command* {
        auto nameStr = std::string(name);

        // Check if it's an alias
        if (auto aliasIt = aliases_.find(nameStr); aliasIt != aliases_.end()) {
            nameStr = aliasIt->second;
        }

        // Find the command
        if (auto it = commands_.find(nameStr); it != commands_.end()) {
            return it->second.get();
        }

        return nullptr;
    }

    /**
     * @brief Get all registered commands
     * @return Vector of command pointers
     */
    [[nodiscard]] auto getAllCommands() const -> std::vector<Command*> {
        std::vector<Command*> result;
        result.reserve(commands_.size());

        for (const auto& [name, cmd] : commands_) {
            result.push_back(cmd.get());
        }

        return result;
    }

    /**
     * @brief Clear all registered commands
     */
    auto clear() -> void {
        commands_.clear();
        aliases_.clear();
    }

    /**
     * @brief Get number of registered commands
     * @return Command count
     */
    [[nodiscard]] auto size() const -> std::size_t { return commands_.size(); }

   private:
    CommandRegistry() = default;

    std::unordered_map<std::string, std::unique_ptr<Command>> commands_;
    std::unordered_map<std::string, std::string> aliases_;
};

/**
 * @brief Parse command line into command name and arguments
 * @param line Input line from user
 * @return Pair of (command_name, arguments)
 */
[[nodiscard]] inline auto parseCommandLine(std::string_view line)
    -> std::pair<std::string, std::vector<std::string>> {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;

    for (char chr : line) {
        if (chr == '"' || chr == '\'') {
            inQuotes = !inQuotes;
        } else if (std::isspace(static_cast<unsigned char>(chr)) && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += chr;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    if (tokens.empty()) {
        return {"", {}};
    }

    std::string commandName = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    return {commandName, args};
}

}  // namespace cli
