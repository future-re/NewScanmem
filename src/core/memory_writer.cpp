#include "memseek/core/memory_writer.hpp"

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

    const auto addresses = resolveMatchAddresses(scanner.getMatches(), indices);
    VecWriteResult summary{};
    summary.results.reserve(indices.size());

    for (std::size_t request = 0; request < indices.size(); ++request) {
        const auto index = indices[request];
        if (!addresses[request]) {
            ++summary.failedCount;
            summary.errors.push_back(
                std::format("match index {} out of range", index));
            continue;
        }

        const auto address = *addresses[request];
        const auto written =
            core::writeBytes(m_pid, std::bit_cast<void*>(address), bytes);
        if (!written || *written != bytes.size()) {
            ++summary.failedCount;
            summary.errors.push_back(
                !written ? std::format("match #{} write failed: {}", index,
                                       written.error())
                         : std::format("match #{} partial write: expected {} "
                                       "bytes, wrote {}",
                                       index, bytes.size(), *written));
            summary.results.push_back({.address = address,
                                       .bytesWritten = written ? *written : 0,
                                       .success = false});
            continue;
        }
        ++summary.successCount;
        summary.results.push_back(
            {.address = address, .bytesWritten = *written, .success = true});
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
auto MemoryWriter::resolveMatchAddresses(
    const scan::MatchesAndOldValuesArray& matches,
    const std::vector<std::size_t>& matchIndices)
    -> std::vector<std::optional<std::uintptr_t>> {
    struct PendingIndex {
        std::size_t target;
        std::size_t request;
    };

    std::vector<PendingIndex> pending;
    pending.reserve(matchIndices.size());
    for (std::size_t request = 0; request < matchIndices.size(); ++request)
        pending.push_back({.target = matchIndices[request], .request = request});
    std::ranges::sort(pending, {}, &PendingIndex::target);

    std::vector<std::optional<std::uintptr_t>> addresses(matchIndices.size());
    std::size_t pendingIndex = 0;
    std::size_t matchIndex = 0;

    for (const auto& swath : matches.swaths) {
        if (pendingIndex >= pending.size()) break;
        if (swath.firstByteInChild == nullptr) continue;
        const auto base = std::bit_cast<std::uintptr_t>(swath.firstByteInChild);

        for (std::size_t offset = 0; offset < swath.data.size(); ++offset) {
            if (swath.data[offset].matchInfo == MatchFlags::EMPTY) continue;

            while (pendingIndex < pending.size() &&
                   pending[pendingIndex].target == matchIndex) {
                addresses[pending[pendingIndex].request] = base + offset;
                ++pendingIndex;
            }
            ++matchIndex;
            if (pendingIndex >= pending.size()) break;
        }
    }

    return addresses;
}
}  // namespace core
