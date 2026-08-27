#include "newscanmem/ui/console.hpp"

#include <cctype>
#include <iostream>

namespace ui {
ConsoleUI::ConsoleUI(const MessageContext context) : m_printer(context) { setDebugMode(context.debugMode); setBackendMode(context.backendMode); }
ConsoleUI::ConsoleUI(const bool debug_mode, const bool backend_mode) : ConsoleUI({.debugMode = debug_mode, .backendMode = backend_mode, .colorMode = true}) {}
void ConsoleUI::print(const std::string_view message) { std::cout << message << '\n'; std::cout.flush(); }
void ConsoleUI::printError(const std::string_view message) { m_printer.error("{}", message); }
void ConsoleUI::printWarning(const std::string_view message) { m_printer.warn("{}", message); }
void ConsoleUI::printInfo(const std::string_view message) { m_printer.info("{}", message); }
void ConsoleUI::printDebug(const std::string_view message) { if (isDebugMode()) m_printer.debug("{}", message); }
auto ConsoleUI::getLine(const std::string_view prompt) -> std::optional<std::string> { return m_lineEditor.readLine(prompt); }
auto ConsoleUI::confirm(const std::string_view message) -> bool { std::cout << message << " (y/n): "; std::cout.flush(); std::string response; if (!std::getline(std::cin, response)) return false; const auto first = response.find_first_not_of(" \t\r\n"); const auto last = response.find_last_not_of(" \t\r\n"); response = first == std::string::npos ? "" : response.substr(first, last - first + 1); for (auto& character : response) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character))); return response == "y" || response == "yes"; }
void ConsoleUI::clearScreen() { std::cout << "\033[2J\033[H"; std::cout.flush(); }
void ConsoleUI::setBackendMode(const bool enabled) { m_backendMode = enabled; m_printer = MessagePrinter({.debugMode = isDebugMode(), .backendMode = enabled}); }
auto ConsoleUI::isBackendMode() const -> bool { return m_backendMode; }
auto ConsoleUI::getPrinter() -> MessagePrinter& { return m_printer; }
void ConsoleUI::setCompletionCallback(CompletionCallback callback) { m_lineEditor.setCompletionCallback(std::move(callback)); }
auto makeConsoleUI() -> ConsoleUI { return ConsoleUI{}; }
auto makeConsoleUI(const bool debug_mode, const bool backend_mode) -> ConsoleUI { return ConsoleUI{debug_mode, backend_mode}; }
}  // namespace ui
