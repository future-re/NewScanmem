module;

#include <sys/types.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <format>
#include <expected>
#include <latch>
#include <span>
#include <thread>
#include <vector>

export module scan.engine;

import scan.factory;
import scan.match_storage;
import scan.types;
import scan.routine;
import value.core;
import value.flags;
import core.maps;
import core.proc_mem;

namespace scan {

using core::ProcMemIO;
using core::Region;

inline void appendBytesToSwath(MatchesAndOldValuesSwath& swath,
                               const std::uint8_t* buffer,
                               std::size_t bytesRead, void* baseAddr) {
    if (swath.data.empty()) {
        swath.firstByteInChild = baseAddr;
    }
    for (std::size_t index = 0; index < bytesRead; ++index) {
        swath.data.push_back({.oldByte = buffer[index],
                              .matchInfo = MatchFlags::EMPTY,
                              .matchLength = 0});
    }
}

[[nodiscard]] inline auto prepareScanRegionsForEngine(pid_t pid,
                                                      const ScanOptions& opts)
    -> std::expected<std::vector<core::Region>, std::string> {
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) {
        return std::unexpected{std::format("readProcessMaps failed: {}",
                                           regionsExp.error().message)};
    }
    auto regions = *regionsExp;
    if (opts.regionFilter.isScanTimeFilter() &&
        opts.regionFilter.filter.isActive()) {
        regions = opts.regionFilter.filter.filterRegions(regions);
    }
    return regions;
}

[[nodiscard]] inline auto prepareScanRoutineForEngine(
    const ScanOptions& opts) -> std::expected<ScanRoutine, std::string> {
    auto routine =
        makeScanRoutine(opts.dataType, opts.matchType, opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }
    return routine;
}

[[nodiscard]] inline auto scanWindowSizeForEngine(const ScanOptions& opts,
                                                  const UserValue* userValue)
    -> std::size_t {
    std::size_t window = bytesNeededForType(opts.dataType);
    if (userValue != nullptr) {
        window = std::max(window, userValue->primary.size());
        if (userValue->secondary) {
            window = std::max(window, userValue->secondary->size());
        }
    }
    return std::max<std::size_t>(1, window);
}

inline auto fetchOldBytes(const MatchesAndOldValuesArray& previous,
                          void* address, std::size_t length,
                          std::vector<std::uint8_t>& out) -> bool {
    if (address == nullptr || length == 0) {
        return false;
    }
    for (const auto& swath : previous.swaths) {
        if (swath.firstByteInChild == nullptr || swath.data.empty()) {
            continue;
        }
        const auto* base =
            static_cast<const std::uint8_t*>(swath.firstByteInChild);
        const auto* current = static_cast<const std::uint8_t*>(address);
        if (current < base) {
            continue;
        }
        const std::size_t offset = static_cast<std::size_t>(current - base);
        if (offset >= swath.data.size()) {
            continue;
        }
        const std::size_t remaining = swath.data.size() - offset;
        if (remaining < length) {
            continue;
        }
        out.resize(length);
        for (std::size_t index = 0; index < length; ++index) {
            out[index] = swath.data[offset + index].oldByte;
        }
        return true;
    }
    return false;
}

inline auto makeOldValue(const MatchesAndOldValuesArray* previousSnapshot,
                         void* address, std::size_t length)
    -> std::optional<Value> {
    if (previousSnapshot == nullptr || address == nullptr || length == 0) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> oldBytes;
    if (!fetchOldBytes(*previousSnapshot, address, length, oldBytes)) {
        return std::nullopt;
    }

    Value oldValue;
    oldValue.bytes = std::move(oldBytes);
    oldValue.flags =
        MatchFlags::B8 | MatchFlags::B16 | MatchFlags::B32 | MatchFlags::B64;
    return oldValue;
}

inline void scanBlock(std::span<const std::uint8_t> buffer,
                      std::size_t baseIndex, std::size_t step,
                      const ScanRoutine& routine, const UserValue* userValue,
                      MatchesAndOldValuesSwath& swath, ScanStats& stats,
                      void* regionBlockBase,
                      const MatchesAndOldValuesArray* previousSnapshot,
                      std::size_t oldSliceLen, bool reverseEndianness) {
    const std::size_t stepSize = std::max<std::size_t>(1, step);
    for (std::size_t offset = 0; offset < buffer.size(); offset += stepSize) {
        auto memory = buffer.subspan(offset);
        auto address = static_cast<void*>(
            static_cast<std::uint8_t*>(regionBlockBase) + offset);
        auto oldValue = makeOldValue(previousSnapshot, address, oldSliceLen);
        auto context = makeScanContext(
            memory, oldValue ? &*oldValue : nullptr, userValue,
            (userValue != nullptr) ? userValue->flag() : MatchFlags::EMPTY,
            reverseEndianness);
        auto result = routine(context);
        if (!result) {
            continue;
        }
        swath.markRangeByIndex(baseIndex + offset, result.matchLength,
                               result.matchedFlag);
        stats.matches++;
    }
}

export inline auto scanRegion(
    const Region& region, ProcMemIO& reader, const ScanOptions& opts,
    const ScanRoutine& routine, const UserValue* userValue, ScanStats& stats,
    const MatchesAndOldValuesArray* previousSnapshot, std::size_t oldSliceLen)
    -> std::optional<MatchesAndOldValuesSwath> {
    if (!region.isReadable() || region.size == 0) {
        return std::nullopt;
    }

    stats.regionsVisited++;
    MatchesAndOldValuesSwath swath;
    std::vector<std::uint8_t> buffer(opts.blockSize);
    std::size_t regionOffset = 0;

    while (regionOffset < region.size) {
        const std::size_t remaining = region.size - regionOffset;
        const std::size_t toRead = std::min(remaining, opts.blockSize);
        auto* baseAddr =
            static_cast<std::uint8_t*>(region.start) + regionOffset;

        auto bytesReadExp = reader.read(baseAddr, buffer.data(), toRead);
        if (!bytesReadExp) {
            regionOffset += toRead;
            continue;
        }

        const std::size_t bytesRead = *bytesReadExp;
        if (bytesRead == 0) {
            regionOffset += toRead;
            continue;
        }

        const std::size_t baseIndex = swath.data.size();
        appendBytesToSwath(swath, buffer.data(), bytesRead, baseAddr);
        scanBlock(std::span<const std::uint8_t>(buffer.data(), bytesRead),
                  baseIndex, opts.step, routine, userValue, swath, stats,
                  baseAddr, previousSnapshot, oldSliceLen,
                  opts.reverseEndianness);

        stats.bytesScanned += bytesRead;
        regionOffset += bytesRead;
    }

    return swath.data.empty() ? std::nullopt : std::make_optional(swath);
}

export inline auto runScanInternal(
    pid_t pid, const ScanOptions& opts, const UserValue* userValue,
    MatchesAndOldValuesArray& out,
    const MatchesAndOldValuesArray* previousSnapshot)
    -> std::expected<ScanStats, std::string> {
    out.swaths.clear();

    auto regionsExp = prepareScanRegionsForEngine(pid, opts);
    if (!regionsExp) {
        return std::unexpected{regionsExp.error()};
    }
    auto regions = std::move(*regionsExp);

    auto routineExp = prepareScanRoutineForEngine(opts);
    if (!routineExp) {
        return std::unexpected{routineExp.error()};
    }
    auto routine = *routineExp;

    ProcMemIO reader{pid};
    if (auto err = reader.open(); !err) {
        return std::unexpected{err.error()};
    }

    ScanStats stats{};
    const std::size_t oldSliceLen = scanWindowSizeForEngine(opts, userValue);

    for (const auto& region : regions) {
        if (auto swath =
                scanRegion(region, reader, opts, routine, userValue, stats,
                           previousSnapshot, oldSliceLen)) {
            out.addSwath(*swath);
        }
    }

    return stats;
}

export [[nodiscard]] inline auto runScan(pid_t pid, const ScanOptions& opts,
                                         const UserValue* userValue,
                                         MatchesAndOldValuesArray& out)
    -> std::expected<ScanStats, std::string> {
    return runScanInternal(pid, opts, userValue, out, nullptr);
}

export [[nodiscard]] inline auto runScanWithPrevious(
    pid_t pid, const ScanOptions& opts, const UserValue* userValue,
    MatchesAndOldValuesArray& out,
    const MatchesAndOldValuesArray& previousSnapshot)
    -> std::expected<ScanStats, std::string> {
    return runScanInternal(pid, opts, userValue, out, &previousSnapshot);
}

// WORKER Thread: From shared queue, grab regions to scan
static void scanRegionsWorker(
    pid_t pid, std::span<const core::Region> regions,
    std::atomic_size_t& nextIndex, const ScanOptions& opts,
    const ScanRoutine& routine, const UserValue* userValue,
    std::size_t oldSlice, const MatchesAndOldValuesArray* previousSnapshot,
    ScanStats& localStats,
    std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>& localSwaths,
    std::latch& workDone) {
    core::ProcMemIO localReader{pid};
    if (auto err = localReader.open(); !err) {
        workDone.count_down();
        return;
    }

    while (true) {
        size_t index = nextIndex.fetch_add(1, std::memory_order_relaxed);
        if (index >= regions.size()) {
            break;
        }
        const auto& region = regions[index];
        if (auto swath =
                scanRegion(region, localReader, opts, routine, userValue,
                           localStats, previousSnapshot, oldSlice)) {
            localSwaths.emplace_back(region.id, *swath);
        }
    }

    workDone.count_down();
}

// Combine all thread scan results into output array
static void mergeThreadResults(
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>&
        threadSwaths,
    std::span<const core::Region> regions, MatchesAndOldValuesArray& out) {
    // Collect all (region.id, swath) from all threads
    std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>> allSwaths;
    size_t totalPairs = 0;
    for (const auto& threadVec : threadSwaths) {
        totalPairs += threadVec.size();
    }
    allSwaths.reserve(totalPairs);
    for (auto& threadVec : threadSwaths) {
        allSwaths.insert(allSwaths.end(),
                         std::make_move_iterator(threadVec.begin()),
                         std::make_move_iterator(threadVec.end()));
    }

    // Sort by region.id to ensure output order matches sequential scan
    std::ranges::sort(allSwaths, [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    // Deduplication guard: avoid merging swaths from the same region multiple
    // times
    std::vector<bool> seen;
    if (!regions.empty()) {
        std::size_t maxRegionId = 0;
        for (const auto& regionItem : regions) {
            maxRegionId = std::max(maxRegionId, regionItem.id);
        }
        seen.assign(maxRegionId + 1, false);
    }
    for (auto& pairItem : allSwaths) {
        std::size_t regionId = pairItem.first;
        if (regionId < seen.size()) {
            if (seen[regionId]) {
                continue;  // skip duplicate
            }
            seen[regionId] = true;
        }
        out.addSwath(pairItem.second);
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
export auto runScanParallel(pid_t pid, const ScanOptions& opts,
                            const UserValue* userValue,
                            MatchesAndOldValuesArray& out,
                            const MatchesAndOldValuesArray* previousSnapshot)
    -> std::expected<ScanStats, std::string> {
    out.swaths.clear();

    auto regionsExp = prepareScanRegionsForEngine(pid, opts);
    if (!regionsExp) {
        return std::unexpected{regionsExp.error()};
    }
    auto regions = *regionsExp;

    if (regions.empty()) {
        return ScanStats{};
    }

    auto routineExp = prepareScanRoutineForEngine(opts);
    if (!routineExp) {
        return std::unexpected{routineExp.error()};
    }
    auto routine = *routineExp;
    const std::size_t oldSlice = scanWindowSizeForEngine(opts, userValue);

    const size_t NUM_THREADS =
        std::min(static_cast<size_t>(std::max(1u, std::thread::hardware_concurrency())),
                 regions.size());

    if (NUM_THREADS <= 1) {
        return runScanInternal(pid, opts, userValue, out, previousSnapshot);
    }

    std::vector<ScanStats> results(NUM_THREADS);
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>
        threadSwaths(NUM_THREADS);
    std::latch workDone{static_cast<ptrdiff_t>(NUM_THREADS)};

    std::vector<std::jthread> threads;
    threads.reserve(NUM_THREADS);

    std::atomic_size_t nextIndex{0};
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            scanRegionsWorker(pid, regions, nextIndex, opts, routine, userValue,
                              oldSlice, previousSnapshot, results[i],
                              threadSwaths[i], workDone);
        });
    }

    workDone.wait();

    mergeThreadResults(threadSwaths, regions, out);

    ScanStats totalStats{};
    for (auto& result : results) {
        totalStats.regionsVisited += result.regionsVisited;
        totalStats.bytesScanned += result.bytesScanned;
        totalStats.matches += result.matches;
    }

    return totalStats;
}

}  // namespace scan
