#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ui {
using CompletionCallback =
    std::function<std::vector<std::string>(std::string_view)>;
class LineEditor {
   public:
    void setCompletionCallback(CompletionCallback callback);
    [[nodiscard]] auto readLine(std::string_view prompt)
        -> std::optional<std::string>;

   private:
    static void drawLine(std::string_view prompt, const std::string& buffer,
                         std::size_t cursor);
    static void applyBackspace(std::string& buffer, std::size_t& cursor);
    static void applyInsert(std::string& buffer, std::size_t& cursor,
                            char character);
    void handleCompletion(std::string& buffer, std::size_t& cursor,
                          std::string_view prompt);
    [[nodiscard]] auto readInteractive(std::string_view prompt)
        -> std::optional<std::string>;
    CompletionCallback m_completionCallback;
};
}  // namespace ui
