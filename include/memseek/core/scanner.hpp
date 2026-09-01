#pragma once

/**
 * @file scanner.hpp
 * @brief High-level scanner interface with explicit operations
 *
 * High-level scanner with explicit scan operations:
 * - snapshot(): Full memory scan (creates baseline)
 * - filter(): Incremental scan on existing matches
 * - rescan(): Clear and perform full scan again
 */

#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <expected>
#include <optional>
#include <string>

#include "memseek/core/maps.hpp"
#include "memseek/core/scan_history.hpp"
#include "memseek/scan/engine.hpp"
#include "memseek/scan/filter.hpp"
#include "memseek/scan/match_storage.hpp"
#include "memseek/scan/types.hpp"
#include "memseek/value/core.hpp"
#include "memseek/value/flags.hpp"

namespace core {

struct ScannerResult {
    ScanStats stats;                   ///< Scan statistics
    std::size_t matchCount{0};         ///< Total matches found
    bool success{false};               ///< Whether scan succeeded
    std::optional<std::string> error;  ///< Error message if failed
};

/**
 * @class Scanner
 * @brief High-level scanner with explicit operations
 *
 */
class Scanner {
   public:
    explicit Scanner(pid_t pid);

    [[nodiscard]] auto snapshot(
        const ScanOptions& opts,
        const std::optional<UserValue>& value = std::nullopt,
        bool saveToHistory = false) -> ScannerResult;

    [[nodiscard]] auto filter(
        const ScanOptions& opts,
        const std::optional<UserValue>& value = std::nullopt,
        bool saveToHistory = false) -> ScannerResult;

    [[nodiscard]] auto rescan(
        const ScanOptions& opts,
        const std::optional<UserValue>& value = std::nullopt,
        bool saveToHistory = false) -> ScannerResult;

    [[nodiscard]] auto getResultCount() const -> std::size_t;
    [[nodiscard]] auto getResult(std::size_t index) const -> const ScanRecord*;
    [[nodiscard]] auto getResults() const -> const std::deque<ScanRecord>&;
    auto clearResultHistory() -> void;

    [[nodiscard]] auto getMatches() const
        -> const scan::MatchesAndOldValuesArray&;

    // Returning mutable storage invalidates the cached match count because the
    // caller may mutate match flags directly.
    [[nodiscard]] auto getMatches() -> scan::MatchesAndOldValuesArray&;

    auto clearMatches() -> void;
    auto reset() -> void;

    [[nodiscard]] auto getMatchCount() const -> std::size_t;
    [[nodiscard]] auto hasMatches() const -> bool;
    [[nodiscard]] auto getPid() const -> pid_t;
    [[nodiscard]] auto getLastDataType() const -> std::optional<ScanDataType>;

   private:
    pid_t m_pid;
    scan::MatchesAndOldValuesArray m_matches;
    ScanHistory m_history;
    std::optional<ScanDataType> m_lastDataType;
    mutable std::optional<std::size_t> m_matchCountCache{0};

    [[nodiscard]] auto doScan(const ScanOptions& opts,
                              const std::optional<UserValue>& value,
                              bool saveToHistory) -> ScannerResult;

    auto saveResultToHistory(const ScanStats& stats, const ScanOptions& opts,
                             const std::optional<UserValue>& value) -> void;
    auto pruneEmptySwaths() -> void;
    auto invalidateMatchCount() noexcept -> void;
};

}  // namespace core
