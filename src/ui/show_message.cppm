module;

#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>

export module ui.show_message;

export namespace ui {

enum class MessageType : uint8_t { INFO, WARN, ERROR, DEBUG, USER };

struct MessageContext {
    bool debugMode = false;
    bool backendMode = false;
    bool colorMode = true;  // 默认启用彩色输出
};

// ANSI 颜色码
namespace color {
constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view BOLD = "\033[1m";
constexpr std::string_view DIM = "\033[2m";

// 前景色
constexpr std::string_view BLACK = "\033[30m";
constexpr std::string_view RED = "\033[31m";
constexpr std::string_view GREEN = "\033[32m";
constexpr std::string_view YELLOW = "\033[33m";
constexpr std::string_view BLUE = "\033[34m";
constexpr std::string_view MAGENTA = "\033[35m";
constexpr std::string_view CYAN = "\033[36m";
constexpr std::string_view WHITE = "\033[37m";

// 明亮前景色
constexpr std::string_view BRIGHT_RED = "\033[91m";
constexpr std::string_view BRIGHT_GREEN = "\033[92m";
constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
constexpr std::string_view BRIGHT_BLUE = "\033[94m";
constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
constexpr std::string_view BRIGHT_CYAN = "\033[96m";
}  // namespace color

class MessagePrinter {
   public:
    MessagePrinter(MessageContext ctx = {}) : m_ctx(ctx) {}

    template <typename... Args>
    void print(MessageType type, std::format_string<Args...> fmt,
               Args&&... args) const {
        std::string msg = std::format(fmt, std::forward<Args>(args)...);

        if (m_ctx.colorMode) {
            switch (type) {
                case MessageType::INFO:
                    std::cerr << std::format("{}{}info:{} {}\n", color::BOLD,
                                             color::BLUE, color::RESET, msg);
                    break;
                case MessageType::WARN:
                    std::cerr << std::format("{}{}warn:{} {}\n", color::BOLD,
                                             color::YELLOW, color::RESET, msg);
                    break;
                case MessageType::ERROR:
                    std::cerr << std::format("{}{}error:{} {}\n", color::BOLD,
                                             color::RED, color::RESET, msg);
                    break;
                case MessageType::DEBUG:
                    if (m_ctx.debugMode) {
                        std::cerr << std::format("{}debug:{} {}\n", color::DIM,
                                                 color::RESET, msg);
                    }
                    break;
                case MessageType::USER:
                    if (!m_ctx.backendMode) {
                        std::cout << msg << '\n';
                    }
                    break;
            }
        } else {
            // 无颜色模式
            switch (type) {
                case MessageType::INFO:
                    std::cerr << std::format("info: {}\n", msg);
                    break;
                case MessageType::WARN:
                    std::cerr << std::format("warn: {}\n", msg);
                    break;
                case MessageType::ERROR:
                    std::cerr << std::format("error: {}\n", msg);
                    break;
                case MessageType::DEBUG:
                    if (m_ctx.debugMode) {
                        std::cerr << std::format("debug: {}\n", msg);
                    }
                    break;
                case MessageType::USER:
                    if (!m_ctx.backendMode) {
                        std::cout << msg << '\n';
                    }
                    break;
            }
        }
    }

    // Convenience member functions
    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) const {
        print(MessageType::INFO, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) const {
        print(MessageType::WARN, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) const {
        print(MessageType::ERROR, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) const {
        print(MessageType::DEBUG, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void user(std::format_string<Args...> fmt, Args&&... args) const {
        print(MessageType::USER, fmt, std::forward<Args>(args)...);
    }

    // Static convenience function for simple string messages (默认带颜色)
    static void info(std::string_view msg) {
        std::cerr << std::format("{}{}info:{} {}\n", color::BOLD, color::BLUE,
                                 color::RESET, msg);
    }
    static void warn(std::string_view msg) {
        std::cerr << std::format("{}{}warn:{} {}\n", color::BOLD, color::YELLOW,
                                 color::RESET, msg);
    }
    static void error(std::string_view msg) {
        std::cerr << std::format("{}{}error:{} {}\n", color::BOLD, color::RED,
                                 color::RESET, msg);
    }
    static void success(std::string_view msg) {
        std::cerr << std::format("{}{}success:{} {}\n", color::BOLD,
                                 color::GREEN, color::RESET, msg);
    }

    // Expose context
    [[nodiscard]] auto conext() const -> const MessageContext& { return m_ctx; }

   private:
    MessageContext m_ctx;
};

}  // namespace ui