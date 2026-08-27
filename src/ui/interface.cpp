#include "newscanmem/ui/interface.hpp"

namespace ui {
void UserInterface::printMessage(const MessageLevel level,
                                 const std::string_view message) {
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
auto UserInterface::readLine(const std::string_view prompt)
    -> std::optional<std::string> {
    return getLine(prompt);
}
void UserInterface::clearScreen() {}
void UserInterface::setDebugMode(const bool enabled) {
    m_debugModeEnabled = enabled;
}
auto UserInterface::isDebugMode() const -> bool { return m_debugModeEnabled; }
}  // namespace ui
