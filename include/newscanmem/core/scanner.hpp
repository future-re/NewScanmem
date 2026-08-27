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

#include "newscanmem/core/maps.hpp"
#include "newscanmem/core/scan_history.hpp"
#include "newscanmem/scan/engine.hpp"
#include "newscanmem/scan/filter.hpp"
#include "newscanmem/scan/match_storage.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

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
    /**
     * @brief Construct scanner for given process
     * @param pid Target process ID
     */
    explicit Scanner(pid_t pid);

    // ====================================================================
    // Explicit Public Operations
    // ====================================================================

    /**
     * @brief Perform a full memory scan (clears existing matches)
     * @param opts Scan options
     * @param value Optional target value
     * @param saveToHistory Whether to save to history
     * @return Scan response with results
     */
    [[nodiscard]] auto snapshot(
        const ScanOptions& opts,
        const std::optional<UserValue>& value = std::nullopt,
        bool saveToHistory = false) -> ScannerResult;

    /**
     * @brief Filter existing matches incrementally
     * @param opts Scan options
     * @param value Optional target value
     * @param saveToHistory Whether to save to history
     * @return Scan response with results
     */
    [[nodiscard]] auto filter(
        const ScanOptions& opts,
        const std::optional<UserValue>& value = std::nullopt,
        bool saveToHistory = false) -> ScannerResult;

    /**
     * @brief Rescan: clear matches and perform full scan
     * @param opts Scan options
     * @param value Optional target value
     * @param saveToHistory Whether to save to history
     * @return Scan response with results
     */
    [[nodiscard]] auto rescan(
        const ScanOptions& opts,
        const std::optional<UserValue>& value = std::nullopt,
        bool saveToHistory = false) -> ScannerResult;

    // ====================================================================
    // State Queries
    // ====================================================================

    /**
     * @brief Get number of saved scan results in history
     */
    [[nodiscard]] auto getResultCount() const -> std::size_t;

    /**
     * @brief Get a specific scan result by index
     */
    [[nodiscard]] auto getResult(std::size_t index) const -> const ScanRecord*;

    /**
     * @brief Get all scan results
     */
    [[nodiscard]] auto getResults() const -> const std::deque<ScanRecord>&;

    /**
     * @brief Clear scan result history
     */
    auto clearResultHistory() -> void;

    /**
     * @brief Get current/active matches from most recent scan
     */
    [[nodiscard]] auto getMatches() const
        -> const scan::MatchesAndOldValuesArray&;

    /**
     * @brief Get mutable current/active matches
     */
    [[nodiscard]] auto getMatches() -> scan::MatchesAndOldValuesArray&;

    /**
     * @brief Clear current/active matches (does not affect result history)
     */
    auto clearMatches() -> void;

    /**
     * @brief Reset scanner state (clears matches and result history)
     */
    auto reset() -> void;

    /**
     * @brief Get number of matches in current scan
     */
    [[nodiscard]] auto getMatchCount() const -> std::size_t;

    /**
     * @brief Check if current scan has matches
     */
    [[nodiscard]] auto hasMatches() const -> bool;

    /**
     * @brief Get target PID
     */
    [[nodiscard]] auto getPid() const -> pid_t;

    /**
     * @brief Get last scan data type
     */
    [[nodiscard]] auto getLastDataType() const -> std::optional<ScanDataType>;

   private:
    pid_t m_pid;
    scan::MatchesAndOldValuesArray m_matches;
    ScanHistory m_history;
    std::optional<ScanDataType> m_lastDataType;

    [[nodiscard]] auto doScan(const ScanOptions& opts,
                              const std::optional<UserValue>& value,
                              bool saveToHistory) -> ScannerResult;

    auto saveResultToHistory(const ScanStats& stats, const ScanOptions& opts,
                             const std::optional<UserValue>& value) -> void;

    auto pruneEmptySwaths() -> void;
};

}  // namespace core
