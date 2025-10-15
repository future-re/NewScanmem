/**
 * @file repl.cppm
 * @brief Read-Eval-Print-Loop for interactive CLI
 */

module;

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module cli.repl;

import cli.command;
import ui.interface;
import ui.show_message;

export namespace cli {

/**
 * @class REPL
 * @brief Read-Eval-Print-Loop for interactive command execution
 */
class REPL {
   public:
    /**
     * @brief Constructor
     * @param ui User interface for input/output
     * @param prompt Prompt string to display
     */
    explicit REPL(std::shared_ptr<ui::UserInterface> ui,
                  std::string_view prompt = "> ")
        : ui_(std::move(ui)), prompt_(prompt) {}

    /**
     * @brief Start the REPL loop
     * @return Exit code (0 for normal exit, non-zero for error)
     */
    auto run() -> int {
        if (!ui_) {
            ui::MessagePrinter::error("No user interface provided");
            return 1;
        }

        ui::MessagePrinter::info("NewScanmem Interactive Console");
        ui::MessagePrinter::info(
            "Type 'help' for available commands, 'quit' to exit\n");

        while (true) {
            // Read command
            auto lineResult = ui_->getLine(prompt_);
            if (!lineResult) {
                // EOF or error
                break;
            }

            std::string line = *lineResult;

            // Skip empty lines
            if (line.empty()) {
                continue;
            }

            // Parse command line
            auto [commandName, args] = parseCommandLine(line);

            if (commandName.empty()) {
                continue;
            }

            // Execute command
            auto result = executeCommand(commandName, args);

            if (!result) {
                ui::MessagePrinter::error("Error: " + result.error());
                continue;
            }

            // Check if we should exit
            if (result->shouldExit) {
                return 0;
            }

            // Display result message if present
            if (!result->message.empty()) {
                if (result->success) {
                    ui::MessagePrinter::info(result->message);
                } else {
                    ui::MessagePrinter::warn(result->message);
                }
            }
        }

        return 0;
    }

    /**
     * @brief Set the prompt string
     * @param prompt New prompt string
     */
    auto setPrompt(std::string_view prompt) -> void { prompt_ = prompt; }

    /**
     * @brief Get the current prompt string
     * @return Current prompt
     */
    [[nodiscard]] auto getPrompt() const -> std::string_view { return prompt_; }

   private:
    /**
     * @brief Execute a command
     * @param commandName Command name
     * @param args Command arguments
     * @return Expected result or error message
     */
    auto executeCommand(const std::string& commandName,
                        const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> {
        auto& registry = CommandRegistry::instance();

        // Find command
        auto* cmd = registry.findCommand(commandName);
        if (!cmd) {
            return std::unexpected("Unknown command: " + commandName +
                                   ". Type 'help' for available commands.");
        }

        // Validate arguments
        auto validationResult = cmd->validateArgs(args);
        if (!validationResult) {
            return std::unexpected(validationResult.error() +
                                   "\nUsage: " + std::string(cmd->getUsage()));
        }

        // Execute command
        return cmd->execute(args);
    }

    std::shared_ptr<ui::UserInterface> ui_;
    std::string prompt_;
};

}  // namespace cli
