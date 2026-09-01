#include "memseek/ui/line_editor.hpp"

#include <unistd.h>

#include <iostream>

#include "memseek/ui/terminal.hpp"

namespace ui {
void LineEditor::setCompletionCallback(CompletionCallback callback) {
    m_completionCallback = std::move(callback);
}
auto LineEditor::readLine(const std::string_view prompt)
    -> std::optional<std::string> {
    if (isatty(STDIN_FILENO) != 0) return readInteractive(prompt);
    if (!prompt.empty()) {
        std::cout << prompt;
        std::cout.flush();
    }
    std::string line;
    return std::getline(std::cin, line)
               ? std::optional<std::string>{std::move(line)}
               : std::nullopt;
}
void LineEditor::drawLine(const std::string_view prompt,
                          const std::string& buffer, const std::size_t cursor) {
    std::cout << "\r" << prompt << buffer << "\x1b[K";
    if (const auto back = buffer.size() - cursor; back != 0)
        std::cout << "\x1b[" << back << "D";
    std::cout.flush();
}
void LineEditor::applyBackspace(std::string& buffer, std::size_t& cursor) {
    if (cursor != 0) {
        buffer.erase(cursor - 1, 1);
        --cursor;
    }
}
void LineEditor::applyInsert(std::string& buffer, std::size_t& cursor,
                             const char character) {
    buffer.insert(cursor++, 1, character);
}
void LineEditor::handleCompletion(std::string& buffer, std::size_t& cursor,
                                  const std::string_view prompt) {
    if (!m_completionCallback) return;
    auto candidates = m_completionCallback(buffer.substr(0, cursor));
    if (candidates.empty()) return;
    if (candidates.size() == 1) {
        buffer = std::move(candidates.front());
        cursor = buffer.size();
        return;
    }
    std::cout << '\n';
    for (const auto& candidate : candidates)
        std::cout << "  " << candidate << '\n';
    drawLine(prompt, buffer, cursor);
}
auto LineEditor::readInteractive(const std::string_view prompt)
    -> std::optional<std::string> {
    RawMode raw;
    if (!raw.enable()) {
        if (!prompt.empty()) {
            std::cout << prompt;
            std::cout.flush();
        }
        std::string line;
        return std::getline(std::cin, line)
                   ? std::optional<std::string>{std::move(line)}
                   : std::nullopt;
    }
    std::string buffer;
    std::size_t cursor = 0;
    drawLine(prompt, buffer, cursor);
    for (;;) {
        const auto event = readKey();
        switch (event.action) {
            case KeyAction::ENTER:
                std::cout << "\r\n";
                std::cout.flush();
                return buffer;
            case KeyAction::BACKSPACE:
                applyBackspace(buffer, cursor);
                break;
            case KeyAction::MOVE_LEFT:
                if (cursor != 0) --cursor;
                break;
            case KeyAction::MOVE_RIGHT:
                if (cursor < buffer.size()) ++cursor;
                break;
            case KeyAction::INSERT_CHAR:
                applyInsert(buffer, cursor, event.character);
                break;
            case KeyAction::TAB_COMPLETE:
                handleCompletion(buffer, cursor, prompt);
                break;
            case KeyAction::EOF_SIGNAL:
                return buffer.empty() ? std::nullopt
                                      : std::optional<std::string>{buffer};
            case KeyAction::IGNORE:
                break;
        }
        drawLine(prompt, buffer, cursor);
    }
}
}  // namespace ui
