/**
 * @file line_editor.cppm
 * @brief Interactive line editor with cursor navigation and tab completion
 */

module;

#include <unistd.h>

#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module ui.line_editor;

import ui.terminal;

export namespace ui {

/**
 * @brief Completion callback function type
 * @param prefix Current input prefix to complete
 * @return Vector of possible completions
 */
using CompletionCallback =
    std::function<std::vector<std::string>(std::string_view)>;

/**
 * @class LineEditor
 * @brief Interactive line editor with cursor movement and tab completion
 */
class LineEditor {
   public:
    LineEditor() = default;

    /**
     * @brief Set completion callback for tab completion
     * @param callback Function returning completions for a given prefix
     */
    void setCompletionCallback(CompletionCallback callback) {
        m_completionCallback = std::move(callback);
    }

    /**
     * @brief Read an interactive line from the terminal
     * @param prompt Prompt string to display
     * @return User input or nullopt on EOF
     */
    [[nodiscard]] auto readLine(std::string_view prompt)
        -> std::optional<std::string> {
        // Non-interactive stdin: fall back to std::getline
        if (isatty(STDIN_FILENO) == 0) {
            if (!prompt.empty()) {
                std::cout << prompt;
                std::cout.flush();
            }
            std::string line;
            if (std::getline(std::cin, line)) {
                return line;
            }
            return std::nullopt;
        }

        // Interactive: use raw mode editing
        return readInteractive(prompt);
    }

   private:
    static void drawLine(std::string_view prompt, const std::string& buffer,
                         std::size_t cursor) {
        std::cout << "\r" << prompt << buffer << "\x1b[K";
        std::size_t backSteps = buffer.size() - cursor;
        if (backSteps > 0) {
            std::cout << "\x1b[" << backSteps << "D";
        }
        std::cout.flush();
    }

    static void applyBackspace(std::string& buffer, std::size_t& cursor) {
        if (cursor > 0) {
            buffer.erase(cursor - 1, 1);
            --cursor;
        }
    }

    static void applyInsert(std::string& buffer, std::size_t& cursor,
                            char character) {
        buffer.insert(cursor, 1, character);
        ++cursor;
    }

    void handleCompletion(std::string& buffer, std::size_t& cursor,
                          std::string_view prompt) {
        if (!m_completionCallback) {
            return;
        }

        std::string prefix = buffer.substr(0, cursor);
        auto candidates = m_completionCallback(prefix);

        if (candidates.empty()) {
            return;
        }

        if (candidates.size() == 1) {
            buffer = candidates[0];
            cursor = buffer.size();
        } else {
            std::cout << "\n";
            for (const auto& candidate : candidates) {
                std::cout << "  " << candidate << "\n";
            }
            drawLine(prompt, buffer, cursor);
        }
    }

    [[nodiscard]] auto readInteractive(std::string_view prompt)
        -> std::optional<std::string> {
        RawMode raw;
        if (!raw.enable()) {
            // Fallback if raw mode fails
            if (!prompt.empty()) {
                std::cout << prompt;
                std::cout.flush();
            }
            std::string line;
            if (std::getline(std::cin, line)) {
                return line;
            }
            return std::nullopt;
        }

        std::string buffer;
        std::size_t cursor = 0;
        drawLine(prompt, buffer, cursor);

        while (true) {
            KeyEvent event = readKey();

            if (event.action == KeyAction::ENTER) {
                std::cout << "\r\n";
                std::cout.flush();
                return buffer;
            }
            if (event.action == KeyAction::BACKSPACE) {
                applyBackspace(buffer, cursor);
            }
            if (event.action == KeyAction::MOVE_LEFT && cursor > 0) {
                --cursor;
            }
            if (event.action == KeyAction::MOVE_RIGHT &&
                cursor < buffer.size()) {
                ++cursor;
            }
            if (event.action == KeyAction::INSERT_CHAR) {
                applyInsert(buffer, cursor, event.character);
            }
            if (event.action == KeyAction::TAB_COMPLETE) {
                handleCompletion(buffer, cursor, prompt);
            }
            if (event.action == KeyAction::EOF_SIGNAL) {
                if (buffer.empty()) {
                    return std::nullopt;
                }
                return buffer;
            }

            drawLine(prompt, buffer, cursor);
        }
    }

    CompletionCallback m_completionCallback;
};

}  // namespace ui
