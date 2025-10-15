/**
 * @file interface.cppm
 * @brief Abstract user interface for scanmem
 */

module;

#include <optional>
#include <string>
#include <string_view>

export module ui.interface;

export namespace ui {

/**
 * @enum MessageLevel
 * @brief Message importance level
 */
enum class MessageLevel { INFO, WARNING, ERROR, DEBUG };

/**
 * @class UserInterface
 * @brief Abstract interface for user interaction
 */
class UserInterface {
   public:
    virtual ~UserInterface() = default;

    /**
     * @brief Print message to user
     * @param message Message to display
     */
    virtual auto print(std::string_view message) -> void = 0;

    /**
     * @brief Print error message
     * @param message Error message
     */
    virtual auto printError(std::string_view message) -> void = 0;

    /**
     * @brief Print warning message
     * @param message Warning message
     */
    virtual auto printWarning(std::string_view message) -> void = 0;

    /**
     * @brief Print informational message
     * @param message Info message
     */
    virtual auto printInfo(std::string_view message) -> void = 0;

    /**
     * @brief Print debug message
     * @param message Debug message
     */
    virtual auto printDebug(std::string_view message) -> void = 0;

    /**
     * @brief Print message with specific level
     * @param level Message level
     * @param message Message content
     */
    virtual auto printMessage(MessageLevel level, std::string_view message)
        -> void {
        switch (level) {
            case MessageLevel::INFO:
                printInfo(message);
                break;
            case MessageLevel::WARNING:
                printWarning(message);
                break;
            case MessageLevel::ERROR:
                printError(message);
                break;
            case MessageLevel::DEBUG:
                printDebug(message);
                break;
        }
    }

    /**
     * @brief Read line from user
     * @param prompt Prompt to display
     * @return User input or nullopt on EOF
     */
    virtual auto getLine(std::string_view prompt)
        -> std::optional<std::string> = 0;

    /**
     * @brief Read line from user (alias for getLine)
     * @param prompt Prompt to display
     * @return User input or nullopt on EOF
     * @deprecated Use getLine instead
     */
    virtual auto readLine(std::string_view prompt)
        -> std::optional<std::string> {
        return getLine(prompt);
    }

    /**
     * @brief Ask user for confirmation
     * @param message Confirmation prompt
     * @return True if user confirms
     */
    virtual auto confirm(std::string_view message) -> bool = 0;

    /**
     * @brief Clear screen
     */
    virtual auto clearScreen() -> void {}

    /**
     * @brief Set debug mode
     * @param enabled True to enable debug messages
     */
    virtual auto setDebugMode(bool enabled) -> void {
        debugModeEnabled_ = enabled;
    }

    /**
     * @brief Check if debug mode is enabled
     * @return True if debug mode enabled
     */
    [[nodiscard]] virtual auto isDebugMode() const -> bool {
        return debugModeEnabled_;
    }

   protected:
    bool debugModeEnabled_ = false;
};

}  // namespace ui
