module;

#include <sys/types.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <expected>
#include <format>
#include <latch>
#include <span>
#include <thread>
#include <vector>

export module scan.co_engine;
import scan.match_storage;
import scan.factory;
import scan.types;
import scan.routine;
import value.flags;
import value.core;
import core.maps;
import core.region_filter;
import core.proc_mem;
import scan.engine;

namespace scan {

// Use the ScanResult from scan.engine (not scan.routine)
using LocalScanResult = ::ScanResult;

// WORKER Thread: From shared queue, grab regions to scan
static void scanRegionsWorker(
    pid_t pid, std::span<const core::Region> regions,
    std::atomic_size_t& nextIndex, const ScanOptions& opts,
    const scanRoutine& routine, const UserValue* userValue, bool usesOld,
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
                           localStats, previousSnapshot, usesOld, oldSlice)) {
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
    // 1. Read all Regions (serial, fast)
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) {
        return std::unexpected{std::format("readProcessMaps failed: {}",
                                           regionsExp.error().message)};
    }
    auto regions = *regionsExp;

    // 2. Apply filtering (serial, fast)
    if (opts.regionFilter.isScanTimeFilter() &&
        opts.regionFilter.filter.isActive()) {
        regions = opts.regionFilter.filter.filterRegions(regions);
    }

    if (regions.empty()) {
        return ScanStats{};
    }

    // 3. Prepare scan routine (serial, fast)
    auto routine = smGetScanroutine(
        opts.dataType, opts.matchType,
        (userValue != nullptr) ? userValue->flag() : MatchFlags::EMPTY,
        opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }

    const bool USES_OLD = matchUsesOldValue(opts.matchType);
    const std::size_t OLD_SLICE = bytesNeededForType(opts.dataType);

    // 4. Determine number of threads
    const size_t NUM_THREADS = std::min(
        (unsigned long)std::thread::hardware_concurrency(),
        regions.size()  // Number of threads should not exceed number of regions
    );

    if (NUM_THREADS <= 1) {
        // Single thread fallback to original implementation
        return runScanInternal(pid, opts, userValue, out, previousSnapshot);
    }

    // 5. Dynamic scheduling: each thread grabs the next region in original
    // order (O(1) scheduling) Note: Compared to taking the minimum among k
    // chunks each time (O(k)) or using a heap (O(log k)),
    //       directly using the original regions + atomic index for work
    //       distribution is simple and balanced.

    // 6. Prepare thread-local storage
    std::vector<LocalScanResult> results(NUM_THREADS);
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>
        threadSwaths(NUM_THREADS);
    std::latch workDone{static_cast<ptrdiff_t>(NUM_THREADS)};

    // 7. Map phase: start concurrent scanning (dynamic region allocation)
    std::vector<std::jthread> threads;
    threads.reserve(NUM_THREADS);

    std::atomic_size_t nextIndex{0};
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            scanRegionsWorker(pid, regions, nextIndex, opts, routine, userValue,
                              USES_OLD, OLD_SLICE, previousSnapshot,
                              results[i].stats, threadSwaths[i], workDone);
        });
    }

    // 8. wait for all workers to finish
    workDone.wait();

    // 9. Reduce phase: merge results
    mergeThreadResults(threadSwaths, regions, out);

    // Accumulate statistics
    ScanStats totalStats{};
    for (auto& result : results) {
        totalStats.regionsVisited += result.stats.regionsVisited;
        totalStats.bytesScanned += result.stats.bytesScanned;
        totalStats.matches += result.stats.matches;
    }

    return totalStats;
}

}  // namespace scan
