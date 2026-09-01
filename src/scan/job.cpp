#include "memseek/scan/job.hpp"

namespace scan {
auto prepareScanRegions(const pid_t pid, const ScanOptions& options)
    -> std::expected<std::vector<core::Region>, std::string> {
    auto regions = readProcessMaps(pid, options.regionLevel);
    if (!regions)
        return std::unexpected(
            std::format("readProcessMaps failed: {}", regions.error().message));
    if (options.regionFilter.isScanTimeFilter() &&
        options.regionFilter.filter.isActive()) {
        return options.regionFilter.filter.filterRegions(*regions);
    }
    return *regions;
}
auto prepareScanRoutine(const ScanOptions& options, const UserValue*)
    -> std::expected<ScanRoutine, std::string> {
    auto routine = makeScanRoutine(options.dataType, options.matchType,
                                   options.reverseEndianness);
    if (!routine) return std::unexpected("no scan routine for options");
    return routine;
}
auto scanWindowSize(const ScanOptions& options,
                    const UserValue* value) -> std::size_t {
    auto window = bytesNeededForType(options.dataType);
    if (value != nullptr) {
        window = std::max(window, value->primary.size());
        if (value->secondary)
            window = std::max(window, value->secondary->size());
    }
    return std::max<std::size_t>(1, window);
}
}  // namespace scan
