#pragma once

/**
 * @file repl.hpp
 * @brief Read-Eval-Print-Loop for interactive CLI
 */

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "newscanmem/cli/command.hpp"
#include "newscanmem/ui/interface.hpp"
#include "newscanmem/ui/show_message.hpp"

namespace cli {

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
    explicit REPL(std::shared_ptr<ui::UserInterface> userInterface,
                  std::string prompt = "> ",
                  std::function<std::string()> promptBuilder = {});

    /**
     * @brief Start the REPL loop
     * @return Exit code (0 for normal exit, non-zero for error)
     */
    auto run() -> int;

    /**
     * @brief Parse and execute a single command line
     * @param line Raw input line
     * @return Expected command result or error
     */
    static auto executeLine(const std::string& line)
        -> std::expected<CommandResult, std::string>;

    /**
     * @brief Set the prompt string
     * @param prompt New prompt string
     */
    auto setPrompt(std::string_view prompt) -> void;

    auto setPromptBuilder(std::function<std::string()> builder) -> void;

    /**
     * @brief Get the current prompt string
     * @return Current prompt
     */
    [[nodiscard]] auto getPrompt() const -> std::string_view;

   private:
    /**
     * @brief Execute a command
     * @param commandName Command name
     * @param args Command arguments
     * @return Expected result or error message
     */
    static auto executeCommand(const std::string& command_name,
                               const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string>;

    std::shared_ptr<ui::UserInterface> m_ui;
    std::string m_prompt;
    std::function<std::string()> m_promptBuilder;
};

}  // namespace cli
