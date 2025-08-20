module;

#include <boost/process.hpp>
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>

export module show_message;

export enum class MessageType : uint8_t { INFO, WARN, ERROR, DEBUG, USER };

export struct MessageContext {
    bool debugMode = false;
    bool backendMode = false;
};

export class MessagePrinter {
   public:
    MessagePrinter(MessageContext ctx = {}) : m_ctx(ctx) {}

    template <typename... Args>
    void print(MessageType type, std::string_view fmt, Args&&... args) const {
        std::string msg = std::vformat(fmt, std::make_format_args(args...));
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

    // 便捷成员函数
    template <typename... Args>
    void info(std::string_view fmt, Args&&... args) const {
        print(MessageType::INFO, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args) const {
        print(MessageType::WARN, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) const {
        print(MessageType::ERROR, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args) const {
        print(MessageType::DEBUG, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void user(std::string_view fmt, Args&&... args) const {
        print(MessageType::USER, fmt, std::forward<Args>(args)...);
    }

    // 暴露上下文
    [[nodiscard]] const MessageContext& conext() const { return m_ctx; }

   private:
    MessageContext m_ctx;
};
