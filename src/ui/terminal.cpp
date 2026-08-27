#include "newscanmem/ui/terminal.hpp"

namespace ui {
auto RawMode::enable() -> bool { if (tcgetattr(STDIN_FILENO, &m_original) == -1) return false; auto raw = m_original; raw.c_lflag &= ~(ICANON | ECHO); raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; return m_active = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0; }
RawMode::~RawMode() { if (m_active) tcsetattr(STDIN_FILENO, TCSANOW, &m_original); }
auto readKey() -> KeyEvent { unsigned char byte{}; if (::read(STDIN_FILENO, &byte, 1) <= 0) return {.action = KeyAction::EOF_SIGNAL}; if (byte == '\n' || byte == '\r') return {.action = KeyAction::ENTER}; if (byte == 0x7f || byte == '\b') return {.action = KeyAction::BACKSPACE}; if (byte == '\t') return {.action = KeyAction::TAB_COMPLETE}; if (byte == 0x04) return {.action = KeyAction::EOF_SIGNAL}; if (byte == '\x1b') { std::array<unsigned char, 2> sequence{}; if (::read(STDIN_FILENO, sequence.data(), 1) == 1 && ::read(STDIN_FILENO, sequence.data() + 1, 1) == 1 && sequence[0] == '[') { if (sequence[1] == 'C') return {.action = KeyAction::MOVE_RIGHT}; if (sequence[1] == 'D') return {.action = KeyAction::MOVE_LEFT}; } return {.action = KeyAction::IGNORE}; } return byte >= 0x20 && byte != 0x7f ? KeyEvent{.action = KeyAction::INSERT_CHAR, .character = static_cast<char>(byte)} : KeyEvent{.action = KeyAction::IGNORE}; }
}  // namespace ui
