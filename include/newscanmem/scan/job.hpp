#pragma once

/**
 * @file job.hpp
 * @brief Shared scan preparation helpers
 */

#include <sys/types.h>

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <vector>

#include "newscanmem/core/maps.hpp"
#include "newscanmem/core/region_filter.hpp"
#include "newscanmem/scan/factory.hpp"
#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"

namespace scan {

[[nodiscard]] auto prepareScanRegions(pid_t pid, const ScanOptions& opts)
    -> std::expected<std::vector<core::Region>, std::string>;

[[nodiscard]] auto prepareScanRoutine(const ScanOptions& opts,
                                      const UserValue* user_value)
    -> std::expected<ScanRoutine, std::string>;

[[nodiscard]] auto scanWindowSize(const ScanOptions& opts,
                                  const UserValue* user_value) -> std::size_t;

}  // namespace scan
