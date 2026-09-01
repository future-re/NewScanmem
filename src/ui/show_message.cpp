#include "memseek/ui/show_message.hpp"

namespace ui {
MessagePrinter::MessagePrinter(const MessageContext context) : m_ctx(context) {}
void MessagePrinter::info(const std::string_view message) {
    std::cerr << std::format("{}{}info:{} {}\n", color::BOLD, color::BLUE,
                             color::RESET, message);
}
void MessagePrinter::warn(const std::string_view message) {
    std::cerr << std::format("{}{}warn:{} {}\n", color::BOLD, color::YELLOW,
                             color::RESET, message);
}
void MessagePrinter::error(const std::string_view message) {
    std::cerr << std::format("{}{}error:{} {}\n", color::BOLD, color::RED,
                             color::RESET, message);
}
void MessagePrinter::success(const std::string_view message) {
    std::cerr << std::format("{}{}success:{} {}\n", color::BOLD, color::GREEN,
                             color::RESET, message);
}
void MessagePrinter::plain(const std::string_view message) {
    std::cerr << message << '\n';
}
auto MessagePrinter::conext() const -> const MessageContext& { return m_ctx; }
}  // namespace ui
