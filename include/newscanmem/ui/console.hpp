#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "newscanmem/ui/interface.hpp"
#include "newscanmem/ui/line_editor.hpp"
#include "newscanmem/ui/show_message.hpp"

namespace ui {
class ConsoleUI : public UserInterface {
 public:
  explicit ConsoleUI(MessageContext context = {});
  explicit ConsoleUI(bool debug_mode, bool backend_mode);
  auto print(std::string_view message) -> void override;
  auto printError(std::string_view message) -> void override;
  auto printWarning(std::string_view message) -> void override;
  auto printInfo(std::string_view message) -> void override;
  auto printDebug(std::string_view message) -> void override;
  auto getLine(std::string_view prompt) -> std::optional<std::string> override;
  auto confirm(std::string_view message) -> bool override;
  auto clearScreen() -> void override;
  auto setBackendMode(bool enabled) -> void;
  [[nodiscard]] auto isBackendMode() const -> bool;
  [[nodiscard]] auto getPrinter() -> MessagePrinter&;
  auto setCompletionCallback(CompletionCallback callback) -> void;
 private:
  MessagePrinter m_printer;
  bool m_backendMode{false};
  LineEditor m_lineEditor;
};
[[nodiscard]] auto makeConsoleUI() -> ConsoleUI;
[[nodiscard]] auto makeConsoleUI(bool debug_mode, bool backend_mode) -> ConsoleUI;
}  // namespace ui
