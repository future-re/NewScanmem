#include "newscanmem/scan/filter.hpp"

#include <span>
#include <vector>

#include "newscanmem/core/proc_mem.hpp"
#include "newscanmem/scan/job.hpp"
#include "newscanmem/scan/routine.hpp"
#include "newscanmem/value/flags.hpp"

namespace {
auto oldValueForCell(const scan::MatchesAndOldValuesSwath& swath,
                     const std::size_t index,
                     const ScanOptions& options) -> std::optional<Value> {
    if (!matchUsesOldValue(options.matchType)) return std::nullopt;
    const auto size = bytesNeededForType(options.dataType);
    if (index > swath.data.size() || size > swath.data.size() - index)
        return std::nullopt;
    Value value;
    value.bytes.reserve(size);
    for (std::size_t offset = 0; offset < size; ++offset)
        value.bytes.push_back(swath.data[index + offset].oldByte);
    value.flags =
        MatchFlags::B8 | MatchFlags::B16 | MatchFlags::B32 | MatchFlags::B64;
    return value;
}
void narrowSwath(scan::MatchesAndOldValuesSwath& swath,
                 const scan::ScanRoutine& routine, const UserValue* value,
                 core::ProcMemIO& reader, std::vector<std::uint8_t>& buffer,
                 ScanStats& stats, const ScanOptions& options) {
    if (swath.firstByteInChild == nullptr) return;
    auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
    for (std::size_t index = 0; index < swath.data.size(); ++index) {
        auto& cell = swath.data[index];
        if (cell.matchInfo == MatchFlags::EMPTY) continue;
        const auto read =
            reader.read(base + index, buffer.data(), buffer.size());
        if (!read || *read == 0) {
            cell.matchInfo = MatchFlags::EMPTY;
            cell.matchLength = 0;
            continue;
        }
        const auto old = oldValueForCell(swath, index, options);
        const auto result = routine(scan::makeScanContext(
            std::span{buffer.data(), *read}, old ? &*old : nullptr, value,
            value ? value->flag() : MatchFlags::EMPTY,
            options.reverseEndianness));
        if (result) {
            cell.matchInfo = result.matchedFlag;
            cell.matchLength = result.matchLength;
            ++stats.matches;
        } else {
            cell.matchInfo = MatchFlags::EMPTY;
            cell.matchLength = 0;
        }
        stats.bytesScanned += *read;
    }
}
}  // namespace
auto filterMatches(const pid_t pid, const ScanOptions& options,
                   const UserValue* value,
                   scan::MatchesAndOldValuesArray& matches)
    -> std::expected<ScanStats, std::string> {
    if (options.step == 0)
        return std::unexpected("step must be greater than zero");
    const auto routine = scan::prepareScanRoutine(options, value);
    if (!routine) return std::unexpected(routine.error());
    core::ProcMemIO reader{pid};
    if (const auto opened = reader.open(); !opened)
        return std::unexpected(opened.error());
    std::vector<std::uint8_t> buffer(scan::scanWindowSize(options, value));
    ScanStats stats;
    for (auto& swath : matches.swaths)
        narrowSwath(swath, *routine, value, reader, buffer, stats, options);
    return stats;
}
