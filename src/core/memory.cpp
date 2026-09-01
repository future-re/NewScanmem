#include "memseek/core/memory.hpp"

namespace core {
auto MemoryWriter::writeBytes(void* address,
                              const std::span<const std::uint8_t> data) const
    -> std::expected<std::size_t, std::string> {
    ProcMemIO memory{m_pid};
    const auto opened = memory.open(true);
    if (!opened)
        return std::unexpected("Failed to open /proc/<pid>/mem for writing: " +
                               opened.error());
    const auto result = memory.write(address, data);
    return result ? result : std::unexpected(result.error());
}
auto MemoryWriter::writeString(void* address, const char* string) const
    -> std::expected<std::size_t, std::string> {
    if (string == nullptr) return std::unexpected("Null string pointer");
    std::size_t length = 0;
    while (string[length] != '\0') ++length;
    return writeBytes(
        address,
        std::span(std::bit_cast<const std::uint8_t*>(string), length + 1));
}
}  // namespace core
