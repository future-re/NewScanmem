/**
 * @file console.cppm
 * @brief Console-based user interface implementation
 */

module;

#include <termios.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module ui.console;

import ui.interface;    // UserInterface, MessageLevel
import ui.show_message; // MessagePrinter, MessageType

export namespace ui {

/**
 * @brief Completion callback function type
 * @param prefix Current input prefix to complete
 * @return Vector of possible completions
 */
using CompletionCallback =
    std::function<std::vector<std::string>(std::string_view)>;

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

        // 非交互式终端使用标准输入
        if (isatty(STDIN_FILENO) == 0) {
            std::string line;
            if (std::getline(std::cin, line)) {
                return line;
            }
            return std::nullopt;
        }

        // 交互式终端使用原始模式编辑
        return readInteractive(prompt);
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

    /**
     * @brief Set completion callback for tab completion
     * @param callback Function to call for getting completions
     */
    auto setCompletionCallback(CompletionCallback callback) -> void {
        m_completionCallback = std::move(callback);
    }

   private:
    // RAII guard for terminal raw mode
    struct RawMode {
        termios original{};
        bool active{false};

        RawMode() = default;
        RawMode(const RawMode&) = delete;
        auto operator=(const RawMode&) -> RawMode& = delete;
        RawMode(RawMode&&) = delete;
        auto operator=(RawMode&&) -> RawMode& = delete;

        auto enable() -> bool {
            if (tcgetattr(STDIN_FILENO, &original) == -1) {
                return false;
            }
            termios raw = original;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            active = (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0);
            return active;
        }

        ~RawMode() {
            if (active) {
                tcsetattr(STDIN_FILENO, TCSANOW, &original);
            }
        }
    };

    enum class KeyAction {
        ENTER,
        BACKSPACE,
        MOVE_LEFT,
        MOVE_RIGHT,
        EOF_SIGNAL,
        INSERT_CHAR,
        TAB_COMPLETE,
        IGNORE
    };

    struct KeyEvent {
        KeyAction action{KeyAction::IGNORE};
        char character{0};
    };

    static auto readKey() -> KeyEvent {
        unsigned char byte = 0;
        ssize_t readResult = ::read(STDIN_FILENO, &byte, 1);

        if (readResult <= 0) {
            return {.action = KeyAction::EOF_SIGNAL, .character = 0};
        }

        if (byte == '\n' || byte == '\r') {
            return {.action = KeyAction::ENTER, .character = 0};
        }
        if (byte == 0x7f || byte == '\b') {
            return {.action = KeyAction::BACKSPACE, .character = 0};
        }
        if (byte == '\t') {  // Tab
            return {.action = KeyAction::TAB_COMPLETE, .character = 0};
        }
        if (byte == 0x04) {  // Ctrl-D
            return {.action = KeyAction::EOF_SIGNAL, .character = 0};
        }
        if (byte == '\x1b') {
            return readEscapeSequence();
        }
        if (byte >= 0x20 && byte != 0x7f) {
            return {.action = KeyAction::INSERT_CHAR,
                    .character = static_cast<char>(byte)};
        }

        return {.action = KeyAction::IGNORE, .character = 0};
    }

    static auto readEscapeSequence() -> KeyEvent {
        std::array<unsigned char, 2> seq{0, 0};
        ssize_t first = ::read(STDIN_FILENO, seq.data(), 1);
        ssize_t second = ::read(STDIN_FILENO, seq.data() + 1, 1);

        if (first == 1 && second == 1 && seq[0] == '[') {
            if (seq[1] == 'C') {
                return {.action = KeyAction::MOVE_RIGHT, .character = 0};
            }
            if (seq[1] == 'D') {
                return {.action = KeyAction::MOVE_LEFT, .character = 0};
            }
        }
        return {.action = KeyAction::IGNORE, .character = 0};
    }

    static auto drawLine(std::string_view prompt, const std::string& buffer,
                         std::size_t cursor) -> void {
        std::cout << "\r" << prompt << buffer << "\x1b[K";
        std::size_t backSteps = buffer.size() - cursor;
        if (backSteps > 0) {
            std::cout << "\x1b[" << backSteps << "D";
        }
        std::cout.flush();
    }

    static auto applyBackspace(std::string& buffer, std::size_t& cursor)
        -> void {
        if (cursor > 0) {
            buffer.erase(cursor - 1, 1);
            --cursor;
        }
    }

    static auto applyInsert(std::string& buffer, std::size_t& cursor,
                            char character) -> void {
        buffer.insert(cursor, 1, character);
        ++cursor;
    }

    auto handleCompletion(std::string& buffer, std::size_t& cursor,
                          std::string_view prompt) -> void {
        if (!m_completionCallback) {
            return;
        }

        // 获取光标前的文本作为补全前缀
        std::string prefix = buffer.substr(0, cursor);
        auto candidates = m_completionCallback(prefix);

        if (candidates.empty()) {
            return;  // 无补全候选
        }

        if (candidates.size() == 1) {
            // 单个候选：自动补全
            buffer = candidates[0];
            cursor = buffer.size();
        } else {
            // 多个候选：显示列表
            std::cout << "\n";
            for (const auto& candidate : candidates) {
                std::cout << "  " << candidate << "\n";
            }
            // 重新绘制提示符和当前输入
            drawLine(prompt, buffer, cursor);
        }
    }

    auto readInteractive(std::string_view prompt)
        -> std::optional<std::string> {
        RawMode raw;
        if (!raw.enable()) {
            std::string line;
            if (std::getline(std::cin, line)) {
                return line;
            }
            return std::nullopt;
        }

        std::string buffer;
        std::size_t cursor = 0;
        drawLine(prompt, buffer, cursor);

        while (true) {
            KeyEvent event = readKey();

            if (event.action == KeyAction::ENTER) {
                std::cout << "\r\n";
                std::cout.flush();
                return buffer;
            }
            if (event.action == KeyAction::BACKSPACE) {
                applyBackspace(buffer, cursor);
            }
            if (event.action == KeyAction::MOVE_LEFT && cursor > 0) {
                --cursor;
            }
            if (event.action == KeyAction::MOVE_RIGHT &&
                cursor < buffer.size()) {
                ++cursor;
            }
            if (event.action == KeyAction::INSERT_CHAR) {
                applyInsert(buffer, cursor, event.character);
            }
            if (event.action == KeyAction::TAB_COMPLETE) {
                handleCompletion(buffer, cursor, prompt);
            }
            if (event.action == KeyAction::EOF_SIGNAL) {
                if (buffer.empty()) {
                    return std::nullopt;
                }
                return buffer;
            }

            drawLine(prompt, buffer, cursor);
        }
    }

    MessagePrinter m_printer;
    bool m_backendMode = false;
    CompletionCallback m_completionCallback;
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
