module;

#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>

export module show_message;

export enum class MessageType : uint8_t { Info, Warn, Error, Debug, User };

export struct MessageContext {
    bool debug_mode = false;
    bool backend_mode = false;
};

export class MessagePrinter {
   public:
    MessagePrinter(MessageContext ctx = {}) : ctx_(ctx) {}

    template <typename... Args>
    void print(MessageType type, std::string_view fmt, Args&&... args) const {
        std::string msg = std::vformat(fmt, std::make_format_args(args...));
        switch (type) {
            case MessageType::Info:
                std::cerr << std::format("info: {}\n", msg);
                break;
            case MessageType::Warn:
                std::cerr << std::format("warn: {}\n", msg);
                break;
            case MessageType::Error:
                std::cerr << std::format("error: {}\n", msg);
                break;
            case MessageType::Debug:
                if (ctx_.debug_mode) {
                    std::cerr << std::format("debug: {}\n", msg);
                }
                break;
            case MessageType::User:
                if (!ctx_.backend_mode) {
                    std::cout << msg << '\n';
                }
                break;
        }
    }

    // 便捷成员函数
    template <typename... Args>
    void info(std::string_view fmt, Args&&... args) const {
        print(MessageType::Info, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args) const {
        print(MessageType::Warn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) const {
        print(MessageType::Error, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args) const {
        print(MessageType::Debug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void user(std::string_view fmt, Args&&... args) const {
        print(MessageType::User, fmt, std::forward<Args>(args)...);
    }

    // 暴露上下文
    [[nodiscard]] const MessageContext& context() const { return ctx_; }

   private:
    MessageContext ctx_;
};