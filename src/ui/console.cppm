/**
 * @file console.cppm
 * @brief Console-based user interface implementation
 *
 * Implements the UserInterface abstract class using stdout/stdin.
 * Delegates interactive line editing to ui.line_editor.
 */

module;

#include <unistd.h>

#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

export module ui.console;

import ui.interface;    // UserInterface, MessageLevel
import ui.show_message; // MessagePrinter, MessageType
import ui.line_editor;  // LineEditor, CompletionCallback

export namespace ui {

// Re-export CompletionCallback so callers importing ui.console still see it
using ui::CompletionCallback;

/**
 * @class ConsoleUI
 * @brief Console-based implementation of UserInterface
 */
class ConsoleUI : public UserInterface {
   public:
    /**
     * @brief Construct console UI
     * @param ctx Message context (debug, backend, color modes)
     */
    explicit ConsoleUI(MessageContext ctx = {}) : m_printer(ctx) {
        setDebugMode(ctx.debugMode);
        setBackendMode(ctx.backendMode);
    }

    /**
     * @brief Legacy constructor for backward compatibility
     * @param debugMode Enable debug output
     * @param backendMode Enable backend mode (machine-readable output)
     */
    explicit ConsoleUI(bool debugMode, bool backendMode)
        : ConsoleUI(MessageContext{.debugMode = debugMode,
                                   .backendMode = backendMode,
                                   .colorMode = true}) {}

    auto print(std::string_view message) -> void override {
        std::cout << message << '\n';
        std::cout.flush();
    }

    auto printError(std::string_view message) -> void override {
        m_printer.error("{}", message);
    }

    auto printWarning(std::string_view message) -> void override {
        m_printer.warn("{}", message);
    }

    auto printInfo(std::string_view message) -> void override {
        m_printer.info("{}", message);
    }

    auto printDebug(std::string_view message) -> void override {
        if (isDebugMode()) {
            m_printer.debug("{}", message);
        }
    }

    auto getLine(std::string_view prompt)
        -> std::optional<std::string> override {
        return m_lineEditor.readLine(prompt);
    }

    auto confirm(std::string_view message) -> bool override {
        std::cout << message << " (y/n): ";
        std::cout.flush();

        std::string response;
        if (!std::getline(std::cin, response)) {
            return false;
        }

        // Trim whitespace and convert to lowercase
        auto trimmed = response;
        while (!trimmed.empty() && (std::isspace(static_cast<unsigned char>(
                                        trimmed.front())) != 0)) {
            trimmed.erase(trimmed.begin());
        }
        while (!trimmed.empty() && (std::isspace(static_cast<unsigned char>(
                                        trimmed.back())) != 0)) {
            trimmed.pop_back();
        }

        for (auto& chr : trimmed) {
            chr = static_cast<char>(
                std::tolower(static_cast<unsigned char>(chr)));
        }

        return trimmed == "y" || trimmed == "yes";
    }

    auto clearScreen() -> void override {
        std::cout << "\033[2J\033[H";
        std::cout.flush();
    }

    auto setBackendMode(bool enabled) -> void {
        m_backendMode = enabled;
        m_printer = MessagePrinter(
            MessageContext{.debugMode = isDebugMode(), .backendMode = enabled});
    }

    [[nodiscard]] auto isBackendMode() const -> bool { return m_backendMode; }

    [[nodiscard]] auto getPrinter() -> MessagePrinter& { return m_printer; }

    /**
     * @brief Set completion callback for tab completion
     * @param callback Function to call for getting completions
     */
    auto setCompletionCallback(CompletionCallback callback) -> void {
        m_lineEditor.setCompletionCallback(std::move(callback));
    }

   private:
    MessagePrinter m_printer;
    bool m_backendMode = false;
    LineEditor m_lineEditor;
};

/**
 * @brief Create console UI with default settings
 * @return Console UI instance
 */
[[nodiscard]] inline auto makeConsoleUI() -> ConsoleUI { return ConsoleUI{}; }

/**
 * @brief Create console UI with specified options
 * @param debugMode Enable debug output
 * @param backendMode Enable backend mode
 * @return Console UI instance
 */
[[nodiscard]] inline auto makeConsoleUI(bool debugMode, bool backendMode)
    -> ConsoleUI {
    return ConsoleUI{debugMode, backendMode};
}

}  // namespace ui
