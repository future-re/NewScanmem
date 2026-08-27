#include "newscanmem/core/memory_writer.hpp"

namespace core {
MemoryWriter::MemoryWriter(const pid_t pid, const utils::Endianness mode)
    : m_pid(pid), m_endianness(mode) {}
void MemoryWriter::setEndianness(const utils::Endianness mode) {
    m_endianness = mode;
}
auto MemoryWriter::getEndianness() const noexcept -> utils::Endianness {
    return m_endianness;
}
auto MemoryWriter::writeToMatch(const Scanner& scanner, const UserValue& value,
                                std::vector<std::size_t> indices)
    -> std::expected<VecWriteResult, std::string> {
    if (m_pid <= 0) return std::unexpected("invalid pid");
    if (indices.empty()) return std::unexpected("no match indices provided");
    const auto bytes = encodeValueBytes(value);
    if (bytes.empty()) return std::unexpected("empty write value");
    VecWriteResult summary{};
    summary.results.reserve(indices.size());
    for (const auto index : indices) {
        const auto address = resolveMatchAddress(scanner.getMatches(), index);
        if (!address) {
            ++summary.failedCount;
            summary.errors.push_back(address.error());
            continue;
        }
        const auto written =
            core::writeBytes(m_pid, std::bit_cast<void*>(*address), bytes);
        if (!written || *written != bytes.size()) {
            ++summary.failedCount;
            summary.errors.push_back(
                !written ? std::format("match #{} write failed: {}", index,
                                       written.error())
                         : std::format("match #{} partial write: expected {} "
                                       "bytes, wrote {}",
                                       index, bytes.size(), *written));
            summary.results.push_back({.address = *address,
                                       .bytesWritten = written ? *written : 0,
                                       .success = false});
            continue;
        }
        ++summary.successCount;
        summary.results.push_back(
            {.address = *address, .bytesWritten = *written, .success = true});
    }
    if (summary.successCount == 0)
        return std::unexpected(summary.errors.empty() ? "no values written"
                                                      : summary.errors.front());
    return summary;
}
auto MemoryWriter::encodeValueBytes(const UserValue& value) const
    -> std::vector<std::uint8_t> {
    auto bytes = value.primary.bytes;
    if (bytes.size() <= 1 || value.primary.flag() == MatchFlags::STRING ||
        value.primary.flag() == MatchFlags::BYTE_ARRAY ||
        m_endianness == utils::getHost())
        return bytes;
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
}
auto MemoryWriter::resolveMatchAddress(
    const scan::MatchesAndOldValuesArray& matches,
    const std::size_t target) -> std::expected<std::uintptr_t, std::string> {
    std::size_t index = 0;
    for (const auto& swath : matches.swaths) {
        if (swath.firstByteInChild == nullptr) continue;
        const auto base = std::bit_cast<std::uintptr_t>(swath.firstByteInChild);
        for (std::size_t offset = 0; offset < swath.data.size(); ++offset)
            if (swath.data[offset].matchInfo != MatchFlags::EMPTY) {
                if (index++ == target) return base + offset;
            }
    }
    return std::unexpected(std::format("match index {} out of range", target));
}
}  // namespace core
