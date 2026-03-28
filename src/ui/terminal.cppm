/**
 * @file terminal.cppm
 * @brief Low-level terminal I/O: raw mode RAII guard and key event reading
 */

module;

#include <termios.h>
#include <unistd.h>

#include <array>
#include <cstdint>

export module ui.terminal;

export namespace ui {

/**
 * @class RawMode
 * @brief RAII guard for terminal raw mode (disables canonical input & echo)
 */
class RawMode {
   public:
    RawMode() = default;
    RawMode(const RawMode&) = delete;
    auto operator=(const RawMode&) -> RawMode& = delete;
    RawMode(RawMode&&) = delete;
    auto operator=(RawMode&&) -> RawMode& = delete;

    /**
     * @brief Enable raw mode on stdin
     * @return True if successfully enabled
     */
    auto enable() -> bool {
        if (tcgetattr(STDIN_FILENO, &m_original) == -1) {
            return false;
        }
        termios raw = m_original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        m_active = (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0);
        return m_active;
    }

    ~RawMode() {
        if (m_active) {
            tcsetattr(STDIN_FILENO, TCSANOW, &m_original);
        }
    }

   private:
    termios m_original{};
    bool m_active{false};
};

/**
 * @enum KeyAction
 * @brief Categorized keyboard input actions
 */
enum class KeyAction : std::uint8_t {
    ENTER,
    BACKSPACE,
    MOVE_LEFT,
    MOVE_RIGHT,
    EOF_SIGNAL,
    INSERT_CHAR,
    TAB_COMPLETE,
    IGNORE
};

/**
 * @struct KeyEvent
 * @brief A single keyboard event with action type and optional character
 */
struct KeyEvent {
    KeyAction action{KeyAction::IGNORE};
    char character{0};
};

/**
 * @brief Read a single key event from stdin (raw mode assumed)
 * @return Parsed KeyEvent
 */
inline auto readKey() -> KeyEvent {
    unsigned char byte = 0;
    ssize_t readResult = ::read(STDIN_FILENO, &byte, 1);

    if (readResult <= 0) {
        return {.action = KeyAction::EOF_SIGNAL, .character = 0};
    }

    if (byte == '\n' || byte == '\r') {
        return {.action = KeyAction::ENTER, .character = 0};
    }
    // NOLINTNEXTLINE(readability-magic-numbers)
    if (byte == 0x7f || byte == '\b') {
        return {.action = KeyAction::BACKSPACE, .character = 0};
    }
    if (byte == '\t') {
        return {.action = KeyAction::TAB_COMPLETE, .character = 0};
    }
    if (byte == 0x04) {  // Ctrl-D
        return {.action = KeyAction::EOF_SIGNAL, .character = 0};
    }
    if (byte == '\x1b') {
        // Read escape sequence
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
    // NOLINTNEXTLINE(readability-magic-numbers)
    if (byte >= 0x20 && byte != 0x7f) {
        return {.action = KeyAction::INSERT_CHAR,
                .character = static_cast<char>(byte)};
    }

    return {.action = KeyAction::IGNORE, .character = 0};
}

}  // namespace ui
