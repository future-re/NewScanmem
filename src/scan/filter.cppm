module;

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <vector>

export module scan.filter;

import scan.types; // ScanOptions / ScanStats / bytesNeededForType / matchUsesOldValue
import scan.job;
import scan.match_storage;
import scan.routine;
import value.core;
import value.flags;
import core.proc_mem; // ProcMemIO

using scan::MatchesAndOldValuesArray;
using scan::MatchesAndOldValuesSwath;
using scan::OldValueAndMatchInfo;

namespace {

[[nodiscard]] inline auto makeOldValueForCell(const MatchesAndOldValuesSwath& swath,
                                              std::size_t index,
                                              const ScanOptions& opts)
    -> std::optional<Value> {
    if (!matchUsesOldValue(opts.matchType)) {
        return std::nullopt;
    }
    const std::size_t NEED = bytesNeededForType(opts.dataType);
    const std::size_t REM = swath.data.size() - index;
    if (REM < NEED) {
        return std::nullopt;
    }

    Value oldValue;
    oldValue.bytes.reserve(NEED);
    for (std::size_t k = 0; k < NEED; ++k) {
        oldValue.bytes.push_back(swath.data[index + k].oldByte);
    }
    oldValue.flags =
        MatchFlags::B8 | MatchFlags::B16 | MatchFlags::B32 | MatchFlags::B64;
    return oldValue;
}

}  // namespace

// Narrow matches for a single swath using the provided routine.
inline void narrowSwath(MatchesAndOldValuesSwath& swath, auto& routine,
                        const UserValue* value, core::ProcMemIO& reader,
                        std::vector<std::uint8_t>& buffer, ScanStats& stats,
                        const ScanOptions& opts) {
    if (swath.firstByteInChild == nullptr) {
        return;
    }
    auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
    for (std::size_t i = 0; i < swath.data.size(); ++i) {
        auto& cell = swath.data[i];
        if (cell.matchInfo == MatchFlags::EMPTY) {
            continue;
        }
        void* addr = static_cast<void*>(base + i);
        auto readExp = reader.read(addr, buffer.data(), buffer.size());
        if (!readExp || *readExp == 0) {
            cell.matchInfo = MatchFlags::EMPTY;
            cell.matchLength = 0;
            continue;
        }
        auto oldValue = makeOldValueForCell(swath, i, opts);
        auto ctx = scan::makeScanContext(
            std::span<const std::uint8_t>(buffer.data(), *readExp),
            oldValue ? &*oldValue : nullptr, value,
            (value != nullptr) ? value->flag() : MatchFlags::EMPTY,
            opts.reverseEndianness);
        auto result = routine(ctx);
        if (result) {
            cell.matchInfo = result.matchedFlag;
            cell.matchLength = static_cast<std::uint16_t>(result.matchLength);
            stats.matches++;
        } else {
            cell.matchInfo = MatchFlags::EMPTY;
            cell.matchLength = 0;
        }
        stats.bytesScanned += *readExp;
    }
}

// Filter existing matches in-place using current scan options/user value.
export [[nodiscard]] inline auto filterMatches(
    pid_t pid, const ScanOptions& opts, const UserValue* value,
    MatchesAndOldValuesArray& matches)
    -> std::expected<ScanStats, std::string> {
    auto routineExp = scan::prepareScanRoutine(opts, value);
    if (!routineExp) {
        return std::unexpected(routineExp.error());
    }
    auto routine = *routineExp;

    core::ProcMemIO reader{pid};
    if (auto err = reader.open(); !err) {
        return std::unexpected(err.error());
    }

    const std::size_t SLICE_SIZE = scan::scanWindowSize(opts, value);
    std::vector<std::uint8_t> buffer(SLICE_SIZE);
    ScanStats stats{};
    for (auto& swath : matches.swaths) {
        narrowSwath(swath, routine, value, reader, buffer, stats, opts);
    }
    return stats;
}
