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
import value.flags;
import utils.mem64;
import value;
import core.maps;
import core.region_filter;
import scan.engine;

namespace scan {

using LocalScanResult = ScanResult;

// 工作线程：从共享队列抢占 region 进行扫描
static void scanRegionsWorker(
    pid_t pid, std::span<const core::Region> regions,
    std::atomic_size_t& nextIndex, const ScanOptions& opts,
    const scanRoutine& routine, const UserValue* userValue, bool usesOld,
    std::size_t oldSlice, const MatchesAndOldValuesArray* previousSnapshot,
    ScanStats& localStats,
    std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>& localSwaths,
    std::latch& workDone) {
    ProcMemReader localReader{pid};
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

// 合并所有线程的扫描结果到输出数组
static void mergeThreadResults(
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>&
        threadSwaths,
    std::span<const core::Region> regions, MatchesAndOldValuesArray& out) {
    // 收集所有线程的 (region.id, swath)
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

    // 按 region.id 排序，确保输出顺序与顺序扫描一致
    std::ranges::sort(allSwaths, [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    // 去重保护：避免同一 region 的 swath 被重复合并
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
    // 1. 读取所有 Region（串行，快速）
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) {
        return std::unexpected{std::format("readProcessMaps failed: {}",
                                           regionsExp.error().message)};
    }
    auto regions = *regionsExp;

    // 2. 应用过滤（串行，快速）
    if (opts.regionFilter.isScanTimeFilter() &&
        opts.regionFilter.filter.isActive()) {
        regions = opts.regionFilter.filter.filterRegions(regions);
    }

    if (regions.empty()) {
        return ScanStats{};
    }

    // 3. 准备扫描例程（串行，快速）
    auto routine = smGetScanroutine(
        opts.dataType, opts.matchType,
        (userValue != nullptr) ? userValue->flag() : MatchFlags::EMPTY,
        opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }

    const bool USES_OLD = matchUsesOldValue(opts.matchType);
    const std::size_t OLD_SLICE = bytesNeededForType(opts.dataType);

    // 4. 确定线程数量
    const size_t NUM_THREADS =
        std::min((unsigned long)std::thread::hardware_concurrency(),
                 regions.size()  // 线程数不超过 region 数量
        );

    if (NUM_THREADS <= 1) {
        // 单线程回退到原始实现
        return runScanInternal(pid, opts, userValue, out, previousSnapshot);
    }

    // 5. 动态调度：每线程按原始顺序抢占下一个 region（O(1) 调度）
    // 说明：相比每次在 k 个 chunk 上取最小（O(k)）或堆（O(log k)），
    //       直接使用原始 regions + 原子索引进行工作分配，简单且均衡。

    // 6. 准备线程本地存储
    std::vector<LocalScanResult> results(NUM_THREADS);
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>
        threadSwaths(NUM_THREADS);
    std::latch workDone{static_cast<ptrdiff_t>(NUM_THREADS)};

    // 7. Map 阶段：启动并发扫描（动态分配 region）
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

    // 8. 等待所有线程完成
    workDone.wait();

    // 9. Reduce 阶段：合并结果
    mergeThreadResults(threadSwaths, regions, out);

    // 累加统计信息
    ScanStats totalStats{};
    for (auto& result : results) {
        totalStats.regionsVisited += result.stats.regionsVisited;
        totalStats.bytesScanned += result.stats.bytesScanned;
        totalStats.matches += result.stats.matches;
    }

    return totalStats;
}

}  // namespace scan