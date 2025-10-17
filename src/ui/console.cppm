/**
 * @file console.cppm
 * @brief Console-based user interface implementation
 */

module;

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

export module ui.console;

import ui.interface;    // UserInterface, MessageLevel
import ui.show_message; // MessagePrinter, MessageType

export namespace ui {

/**
 * @class ConsoleUI
 * @brief Console-based implementation of UserInterface
 */
class ConsoleUI : public UserInterface {
   public:
    /**
     * @brief Construct console UI
     * @param debugMode Enable debug output
     * @param backendMode Enable backend mode (machine-readable output)
     */
    explicit ConsoleUI(bool debugMode = false, bool backendMode = false)
        : m_printer(MessageContext{.debugMode = debugMode,
                                   .backendMode = backendMode}) {
        setDebugMode(debugMode);
        setBackendMode(backendMode);
    }

    /**
     * @brief Print plain message
     * @param message Message to print
     */
    auto print(std::string_view message) -> void override {
        std::cout << message << '\n';
        std::cout.flush();
    }

    /**
     * @brief Print error message
     * @param message Error message
     */
    auto printError(std::string_view message) -> void override {
        m_printer.error("{}", message);
    }

    /**
     * @brief Print warning message
     * @param message Warning message
     */
    auto printWarning(std::string_view message) -> void override {
        m_printer.warn("{}", message);
    }

    /**
     * @brief Print info message
     * @param message Info message
     */
    auto printInfo(std::string_view message) -> void override {
        m_printer.info("{}", message);
    }

    /**
     * @brief Print debug message
     * @param message Debug message
     */
    auto printDebug(std::string_view message) -> void override {
        if (isDebugMode()) {
            m_printer.debug("{}", message);
        }
    }

    /**
     * @brief Read line from stdin
     * @param prompt Prompt to display
     * @return User input or nullopt on EOF
     */
    auto getLine(std::string_view prompt)
        -> std::optional<std::string> override {
        if (!prompt.empty()) {
            std::cout << prompt;
            std::cout.flush();
        }

        std::string line;
        if (std::getline(std::cin, line)) {
            return line;
        }

        return std::nullopt;  // EOF
    }

    /**
     * @brief Ask for yes/no confirmation
     * @param message Confirmation prompt
     * @return True if user confirms (y/yes)
     */
    auto confirm(std::string_view message) -> bool override {
        std::cout << message << " (y/n): ";
        std::cout.flush();

        std::string response;
        if (!std::getline(std::cin, response)) {
            return false;  // EOF means no
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

    /**
     * @brief Clear terminal screen
     */
    auto clearScreen() -> void override {
        // ANSI escape code to clear screen
        std::cout << "\033[2J\033[H";
        std::cout.flush();
    }

    /**
     * @brief Set backend mode
     * @param enabled True for machine-readable output
     */
    auto setBackendMode(bool enabled) -> void {
        m_backendMode = enabled;
        m_printer = MessagePrinter(
            MessageContext{.debugMode = isDebugMode(), .backendMode = enabled});
    }

    /**
     * @brief Check if backend mode is enabled
     * @return True if backend mode enabled
     */
    [[nodiscard]] auto isBackendMode() const -> bool { return m_backendMode; }

    /**
     * @brief Get underlying message printer
     * @return Reference to MessagePrinter
     */
    [[nodiscard]] auto getPrinter() -> MessagePrinter& { return m_printer; }

   private:
    MessagePrinter m_printer;
    bool m_backendMode = false;
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
