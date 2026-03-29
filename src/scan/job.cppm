/**
 * @file job.cppm
 * @brief Shared scan preparation helpers
 */

module;

#include <sys/types.h>

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <vector>

export module scan.job;

import scan.types;
import scan.factory;
import scan.routine;
import core.maps;
import core.region_filter;
import value.core;

export namespace scan {

[[nodiscard]] inline auto prepareScanRegions(pid_t pid,
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

[[nodiscard]] inline auto prepareScanRoutine(const ScanOptions& opts,
                                             const UserValue* userValue)
    -> std::expected<ScanRoutine, std::string> {
    auto routine =
        makeScanRoutine(opts.dataType, opts.matchType, opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }
    return routine;
}

[[nodiscard]] inline auto scanWindowSize(const ScanOptions& opts,
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

}  // namespace scan
