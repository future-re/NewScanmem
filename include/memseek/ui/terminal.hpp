#pragma once

/**
 * @file terminal.hpp
 * @brief Low-level terminal I/O: raw mode RAII guard and key event reading
 */

#include <termios.h>
#include <unistd.h>

#include <array>
#include <cstdint>

namespace ui {

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
    auto enable() -> bool;

    ~RawMode();

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
[[nodiscard]] auto readKey() -> KeyEvent;

}  // namespace ui
