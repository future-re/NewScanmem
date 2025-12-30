module;

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

export module scan.filter;

import scan.types; // ScanDataType / ScanMatchType / bytesNeededForType / matchUsesOldValue
import scan.factory; // smGetScanroutine
import scan.engine;  // ProcMemReader / ScanStats / ScanOptions
import scan.match_storage;
import utils.mem64;
import value;

using scan::MatchesAndOldValuesArray;
using scan::MatchesAndOldValuesSwath;
using scan::OldValueAndMatchInfo;

// Narrow matches for a single swath using the provided routine.
inline void narrowSwath(MatchesAndOldValuesSwath& swath, auto& routine,
                        const UserValue* value, ProcMemReader& reader,
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
            continue;
        }
        Mem64 mem{buffer.data(), *readExp};
        MatchFlags newFlags = MatchFlags::EMPTY;
        const Value* oldValuePtr = nullptr;
        Value oldValueHolder;
        if (matchUsesOldValue(opts.matchType)) {
            const std::size_t NEED = bytesNeededForType(opts.dataType);
            const std::size_t REM = swath.data.size() - i;
            if (REM >= NEED) {
                oldValueHolder.bytes.resize(NEED);
                for (std::size_t k = 0; k < NEED; ++k) {
                    oldValueHolder.bytes[k] = swath.data[i + k].oldByte;
                }
                oldValueHolder.flags = MatchFlags::B8 | MatchFlags::B16 |
                                       MatchFlags::B32 | MatchFlags::B64;
                oldValuePtr = &oldValueHolder;
            }
        }
        const unsigned MATCHED_LEN =
            routine(&mem, *readExp, oldValuePtr, value, &newFlags);
        if (MATCHED_LEN > 0) {
            cell.matchInfo = newFlags;
            stats.matches++;
        } else {
            cell.matchInfo = MatchFlags::EMPTY;
        }
        stats.bytesScanned += *readExp;
    }
}

// Filter existing matches in-place using current scan options/user value.
export [[nodiscard]] inline auto filterMatches(
    pid_t pid, const ScanOptions& opts, const UserValue* value,
    MatchesAndOldValuesArray& matches)
    -> std::expected<ScanStats, std::string> {
    auto routine =
        smGetScanroutine(opts.dataType, opts.matchType,
                         (value != nullptr) ? value->flag() : MatchFlags::EMPTY,
                         opts.reverseEndianness);
    if (!routine) {
        return std::unexpected("no scan routine for filtered options");
    }

    ProcMemReader reader{pid};
    if (auto err = reader.open(); !err) {
        return std::unexpected(err.error());
    }

    const std::size_t SLICE_SIZE = bytesNeededForType(opts.dataType);
    std::vector<std::uint8_t> buffer(SLICE_SIZE);
    ScanStats stats{};
    for (auto& swath : matches.swaths) {
        narrowSwath(swath, routine, value, reader, buffer, stats, opts);
    }
    return stats;
}
