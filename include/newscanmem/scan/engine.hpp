#pragma once

#include <cstdint>
#include <expected>
#include <string>

#include "newscanmem/scan/match_storage.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"

namespace scan {
[[nodiscard]] auto runScan(pid_t pid, const ScanOptions& options, const UserValue* user_value,
                           MatchesAndOldValuesArray& output) -> std::expected<ScanStats, std::string>;
[[nodiscard]] auto runScanWithPrevious(pid_t pid, const ScanOptions& options, const UserValue* user_value,
    MatchesAndOldValuesArray& output, const MatchesAndOldValuesArray& previous_snapshot) -> std::expected<ScanStats, std::string>;
[[nodiscard]] auto runScanParallel(pid_t pid, const ScanOptions& options, const UserValue* user_value,
    MatchesAndOldValuesArray& output, const MatchesAndOldValuesArray* previous_snapshot) -> std::expected<ScanStats, std::string>;
}  // namespace scan
