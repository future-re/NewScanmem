#pragma once

/**
 * @file command.hpp
 * @brief Command pattern base classes and registry for CLI
 */

#include <cctype>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cli {

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
    Command() = default;
    Command(const Command&) = delete;
    auto operator=(const Command&) -> Command& = delete;
    Command(Command&&) noexcept = default;
    auto operator=(Command&&) noexcept -> Command& = default;
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
    [[nodiscard]] virtual auto getAliases() const -> std::vector<std::string_view>;

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
    [[nodiscard]] virtual auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string>;
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
    static auto instance() -> CommandRegistry&;

    /**
     * @brief Register a command
     * @param command Unique pointer to command
     */
    auto registerCommand(std::unique_ptr<Command> command) -> void;

    /**
     * @brief Find command by name or alias
     * @param name Command name or alias
     * @return Pointer to command, or nullptr if not found
     */
    [[nodiscard]] auto findCommand(std::string_view name) -> Command*;

    /**
     * @brief Get all registered commands
     * @return Vector of command pointers
     */
    [[nodiscard]] auto getAllCommands() const -> std::vector<Command*>;

    /**
     * @brief Clear all registered commands
     */
    auto clear() -> void;

    /**
     * @brief Get number of registered commands
     * @return Command count
     */
    [[nodiscard]] auto size() const -> std::size_t;

   private:
    CommandRegistry() = default;

    std::unordered_map<std::string, std::unique_ptr<Command>> m_commands;
    std::unordered_map<std::string, std::string> m_aliases;
};

/**
 * @brief Parse command line into command name and arguments
 * @param line Input line from user
 * @return Pair of (command_name, arguments)
 */
[[nodiscard]] auto parseCommandLine(std::string_view line) -> std::pair<std::string, std::vector<std::string>>;

}  // namespace cli
