#include "memseek/scan/engine.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <latch>
#include <span>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "memseek/core/proc_mem.hpp"
#include "memseek/scan/job.hpp"
#include "memseek/scan/routine.hpp"
#include "memseek/scan/string.hpp"

namespace scan {
namespace {
using core::ProcMemIO;
using core::Region;

void appendBytes(MatchesAndOldValuesSwath& swath, const std::uint8_t* buffer,
                 const std::size_t count, void* base) {
    if (swath.data.empty()) swath.firstByteInChild = base;
    for (std::size_t index = 0; index < count; ++index)
        swath.data.push_back({.oldByte = buffer[index],
                              .matchInfo = MatchFlags::EMPTY,
                              .matchLength = 0});
}

auto oldValue(const MatchesAndOldValuesArray* previous, void* address,
              const std::size_t length) -> std::optional<Value> {
    if (previous == nullptr || address == nullptr || length == 0)
        return std::nullopt;
    const auto target = std::bit_cast<std::uintptr_t>(address);
    for (const auto& swath : previous->swaths) {
        if (swath.firstByteInChild == nullptr || swath.data.empty()) continue;
        const auto begin =
            std::bit_cast<std::uintptr_t>(swath.firstByteInChild);
        if (target < begin) continue;
        const auto offset = static_cast<std::size_t>(target - begin);
        if (offset > swath.data.size() || length > swath.data.size() - offset)
            continue;
        Value value;
        value.bytes.resize(length);
        for (std::size_t index = 0; index < length; ++index)
            value.bytes[index] = swath.data[offset + index].oldByte;
        value.flags = MatchFlags::B8 | MatchFlags::B16 | MatchFlags::B32 |
                      MatchFlags::B64;
        return value;
    }
    return std::nullopt;
}

void scanRegexBlock(const std::span<const std::uint8_t> buffer,
                    const std::size_t candidateCount,
                    const std::size_t baseIndex, const std::size_t step,
                    const UserValue* userValue,
                    MatchesAndOldValuesSwath& swath, ScanStats& stats) {
    if (userValue == nullptr ||
        userValue->primary.flag() != MatchFlags::STRING ||
        userValue->primary.bytes.empty())
        return;

    const auto pattern = std::string_view{
        std::bit_cast<const char*>(userValue->primary.bytes.data()),
        userValue->primary.bytes.size()};
    const auto stride = std::max<std::size_t>(1, step);

    for (const auto& match : findRegexMatches(buffer, pattern)) {
        if (match.offset >= candidateCount) break;
        if ((match.offset % stride) != 0) continue;
        swath.markRangeByIndex(baseIndex + match.offset, match.length,
                               MatchFlags::B8);
        ++stats.matches;
    }
}

void scanBlock(const std::span<const std::uint8_t> buffer,
               const std::size_t candidateCount,
               const std::size_t baseIndex, const std::size_t step,
               const ScanRoutine& routine, const UserValue* userValue,
               MatchesAndOldValuesSwath& swath, ScanStats& stats,
               void* blockBase, const MatchesAndOldValuesArray* previous,
               const std::size_t oldLength, const bool reverse,
               const bool regexBlockMode) {
    if (regexBlockMode) {
        scanRegexBlock(buffer, candidateCount, baseIndex, step, userValue, swath,
                       stats);
        return;
    }

    const auto stride = std::max<std::size_t>(1, step);
    const auto scanEnd = std::min(candidateCount, buffer.size());
    for (std::size_t offset = 0; offset < scanEnd; offset += stride) {
        auto* address =
            static_cast<void*>(static_cast<std::uint8_t*>(blockBase) + offset);
        const auto old = oldValue(previous, address, oldLength);
        const auto result = routine(makeScanContext(
            buffer.subspan(offset), old ? &*old : nullptr, userValue,
            userValue ? userValue->flag() : MatchFlags::EMPTY, reverse));
        if (!result) continue;
        swath.markRangeByIndex(baseIndex + offset, result.matchLength,
                               result.matchedFlag);
        ++stats.matches;
    }
}

auto scanRegion(
    const Region& region, ProcMemIO& reader, const ScanOptions& options,
    const ScanRoutine& routine, const UserValue* userValue, ScanStats& stats,
    const MatchesAndOldValuesArray* previous,
    const std::size_t oldLength) -> std::optional<MatchesAndOldValuesSwath> {
    if (!region.isReadable() || region.size == 0) return std::nullopt;
    ++stats.regionsVisited;
    MatchesAndOldValuesSwath swath;

    const auto overlap = oldLength > 0 ? oldLength - 1 : 0;
    std::vector<std::uint8_t> buffer(options.blockSize + overlap);
    const bool regexBlockMode = options.dataType == ScanDataType::STRING &&
                                options.matchType == ScanMatchType::MATCH_REGEX;

    for (std::size_t offset = 0; offset < region.size;) {
        const auto primaryCount =
            std::min(region.size - offset, options.blockSize);
        const auto toRead =
            std::min(region.size - offset, primaryCount + overlap);
        auto* base = static_cast<std::uint8_t*>(region.start) + offset;
        const auto read = reader.read(base, buffer.data(), toRead);
        if (!read || *read == 0) {
            offset += primaryCount;
            continue;
        }

        const auto appended = std::min(*read, primaryCount);
        const auto baseIndex = swath.data.size();
        appendBytes(swath, buffer.data(), appended, base);
        scanBlock(std::span{buffer.data(), *read}, appended, baseIndex,
                  options.step, routine, userValue, swath, stats, base, previous,
                  oldLength, options.reverseEndianness, regexBlockMode);
        stats.bytesScanned += appended;
        offset += appended;
    }

    return swath.data.empty()
               ? std::nullopt
               : std::optional<MatchesAndOldValuesSwath>{std::move(swath)};
}

auto scanInternal(const pid_t pid, const ScanOptions& options,
                  const UserValue* userValue, MatchesAndOldValuesArray& output,
                  const MatchesAndOldValuesArray* previous)
    -> std::expected<ScanStats, std::string> {
    if (options.blockSize == 0)
        return std::unexpected("blockSize must be greater than zero");
    if (options.step == 0)
        return std::unexpected("step must be greater than zero");
    output.swaths.clear();
    const auto regions = prepareScanRegions(pid, options);
    if (!regions) return std::unexpected(regions.error());
    const auto routine = prepareScanRoutine(options, userValue);
    if (!routine) return std::unexpected(routine.error());
    ProcMemIO reader{pid};
    if (const auto opened = reader.open(); !opened)
        return std::unexpected(opened.error());
    ScanStats stats;
    const auto oldLength = scanWindowSize(options, userValue);
    for (const auto& region : *regions)
        if (auto swath = scanRegion(region, reader, options, *routine, userValue,
                                    stats, previous, oldLength))
            output.addSwath(*swath);
    return stats;
}

void worker(
    const pid_t pid, const std::span<const Region> regions,
    std::atomic_size_t& next, const ScanOptions& options,
    const ScanRoutine& routine, const UserValue* userValue,
    const std::size_t oldLength, const MatchesAndOldValuesArray* previous,
    ScanStats& stats,
    std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>& swaths,
    std::latch& completed) {
    ProcMemIO reader{pid};
    if (!reader.open()) {
        completed.count_down();
        return;
    }
    for (;;) {
        const auto index = next.fetch_add(1, std::memory_order_relaxed);
        if (index >= regions.size()) break;
        if (auto swath = scanRegion(regions[index], reader, options, routine,
                                    userValue, stats, previous, oldLength))
            swaths.emplace_back(regions[index].id, std::move(*swath));
    }
    completed.count_down();
}

void merge(
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>&
        inputs,
    MatchesAndOldValuesArray& output) {
    std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>> all;
    for (auto& input : inputs)
        all.insert(all.end(), std::make_move_iterator(input.begin()),
                   std::make_move_iterator(input.end()));
    std::ranges::sort(all, {},
                      &std::pair<std::size_t, MatchesAndOldValuesSwath>::first);
    std::size_t last = static_cast<std::size_t>(-1);
    for (auto& [id, swath] : all)
        if (id != std::exchange(last, id)) output.addSwath(std::move(swath));
}
}  // namespace

auto runScan(const pid_t pid, const ScanOptions& options,
             const UserValue* userValue, MatchesAndOldValuesArray& output)
    -> std::expected<ScanStats, std::string> {
    return scanInternal(pid, options, userValue, output, nullptr);
}

auto runScanWithPrevious(const pid_t pid, const ScanOptions& options,
                         const UserValue* userValue,
                         MatchesAndOldValuesArray& output,
                         const MatchesAndOldValuesArray& previous)
    -> std::expected<ScanStats, std::string> {
    return scanInternal(pid, options, userValue, output, &previous);
}

auto runScanParallel(const pid_t pid, const ScanOptions& options,
                     const UserValue* userValue,
                     MatchesAndOldValuesArray& output,
                     const MatchesAndOldValuesArray* previous)
    -> std::expected<ScanStats, std::string> {
    if (options.blockSize == 0)
        return std::unexpected("blockSize must be greater than zero");
    if (options.step == 0)
        return std::unexpected("step must be greater than zero");
    output.swaths.clear();
    const auto regions = prepareScanRegions(pid, options);
    if (!regions) return std::unexpected(regions.error());
    if (regions->empty()) return ScanStats{};
    const auto routine = prepareScanRoutine(options, userValue);
    if (!routine) return std::unexpected(routine.error());
    const auto count = std::min<std::size_t>(
        std::max(1U, std::thread::hardware_concurrency()), regions->size());
    if (count <= 1)
        return scanInternal(pid, options, userValue, output, previous);
    const auto oldLength = scanWindowSize(options, userValue);
    std::vector<ScanStats> stats(count);
    std::vector<std::vector<std::pair<std::size_t, MatchesAndOldValuesSwath>>>
        swaths(count);
    std::latch done{static_cast<std::ptrdiff_t>(count)};
    std::atomic_size_t next{};
    std::vector<std::jthread> threads;
    threads.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
        threads.emplace_back([&, index] {
            worker(pid, *regions, next, options, *routine, userValue, oldLength,
                   previous, stats[index], swaths[index], done);
        });
    done.wait();
    merge(swaths, output);
    ScanStats total;
    for (const auto& item : stats) {
        total.regionsVisited += item.regionsVisited;
        total.bytesScanned += item.bytesScanned;
        total.matches += item.matches;
    }
    return total;
}
}  // namespace scan
