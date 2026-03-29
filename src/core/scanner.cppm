/**
 * @file scanner.cppm
 * @brief High-level scanner interface with explicit operations
 *
 * High-level scanner with explicit scan operations:
 * - snapshot(): Full memory scan (creates baseline)
 * - filter(): Incremental scan on existing matches
 * - rescan(): Clear and perform full scan again
 */

module;

#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <expected>
#include <optional>
#include <string>

export module core.scanner;

import scan.engine;        // runScanParallel
import scan.types;         // ScanDataType, ScanMatchType
import scan.filter;        // filterMatches
import scan.match_storage; // MatchesAndOldValuesArray
import value.core;         // UserValue, Value
import value.flags;
import core.maps;         // RegionScanLevel
import core.scan_history; // ScanHistory

export namespace core {

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
    explicit Scanner(pid_t pid) : m_pid(pid) {}

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
        bool saveToHistory = false) -> ScannerResult {
        clearMatches();
        return doScan(opts, value, saveToHistory);
    }

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
        bool saveToHistory = false) -> ScannerResult {
        if (!hasMatches()) {
            return ScannerResult{
                .stats = {},
                .matchCount = 0,
                .success = false,
                .error =
                    "No existing matches to filter. Run snapshot() first."};
        }

        auto statsExp =
            filterMatches(m_pid, opts, value ? &*value : nullptr, m_matches);
        if (!statsExp) {
            return ScannerResult{.stats = {},
                                 .matchCount = 0,
                                 .success = false,
                                 .error = statsExp.error()};
        }

        pruneEmptySwaths();

        if (saveToHistory) {
            saveResultToHistory(*statsExp, opts, value);
        }

        return ScannerResult{
            .stats = *statsExp, .matchCount = getMatchCount(), .success = true};
    }

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
        bool saveToHistory = false) -> ScannerResult {
        reset();
        return doScan(opts, value, saveToHistory);
    }

    // ====================================================================
    // State Queries
    // ====================================================================

    /**
     * @brief Get number of saved scan results in history
     */
    [[nodiscard]] auto getResultCount() const -> std::size_t {
        return m_history.count();
    }

    /**
     * @brief Get a specific scan result by index
     */
    [[nodiscard]] auto getResult(std::size_t index) const -> const ScanRecord* {
        return m_history.get(index);
    }

    /**
     * @brief Get all scan results
     */
    [[nodiscard]] auto getResults() const -> const std::deque<ScanRecord>& {
        return m_history.getAll();
    }

    /**
     * @brief Clear scan result history
     */
    auto clearResultHistory() -> void { m_history.clear(); }

    /**
     * @brief Get current/active matches from most recent scan
     */
    [[nodiscard]] auto getMatches() const
        -> const scan::MatchesAndOldValuesArray& {
        return m_matches;
    }

    /**
     * @brief Get mutable current/active matches
     */
    [[nodiscard]] auto getMatches() -> scan::MatchesAndOldValuesArray& {
        return m_matches;
    }

    /**
     * @brief Clear current/active matches (does not affect result history)
     */
    auto clearMatches() -> void { m_matches.swaths.clear(); }

    /**
     * @brief Reset scanner state (clears matches and result history)
     */
    auto reset() -> void {
        m_matches.swaths.clear();
        m_history.clear();
    }

    /**
     * @brief Get number of matches in current scan
     */
    [[nodiscard]] auto getMatchCount() const -> std::size_t {
        std::size_t count = 0;
        for (const auto& swath : m_matches.swaths) {
            for (const auto& element : swath.data) {
                if (element.matchInfo != MatchFlags::EMPTY) {
                    ++count;
                }
            }
        }
        return count;
    }

    /**
     * @brief Check if current scan has matches
     */
    [[nodiscard]] auto hasMatches() const -> bool {
        for (const auto& swath : m_matches.swaths) {
            for (const auto& element : swath.data) {
                if (element.matchInfo != MatchFlags::EMPTY) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Get target PID
     */
    [[nodiscard]] auto getPid() const -> pid_t { return m_pid; }

    /**
     * @brief Get last scan data type
     */
    [[nodiscard]] auto getLastDataType() const -> std::optional<ScanDataType> {
        return m_lastDataType;
    }

   private:
    pid_t m_pid;
    scan::MatchesAndOldValuesArray m_matches;
    ScanHistory m_history;
    std::optional<ScanDataType> m_lastDataType;

    [[nodiscard]] auto doScan(const ScanOptions& opts,
                              const std::optional<UserValue>& value,
                              bool saveToHistory) -> ScannerResult {
        m_lastDataType = opts.dataType;
        auto result = runScanParallel(m_pid, opts, value ? &*value : nullptr,
                                      m_matches, nullptr);
        if (!result) {
            return ScannerResult{.stats = {},
                                 .matchCount = 0,
                                 .success = false,
                                 .error = result.error()};
        }

        if (saveToHistory) {
            saveResultToHistory(*result, opts, value);
        }

        return ScannerResult{
            .stats = *result, .matchCount = getMatchCount(), .success = true};
    }

    auto saveResultToHistory(const ScanStats& stats, const ScanOptions& opts,
                             const std::optional<UserValue>& value) -> void {
        ScanRecord record;
        record.stats = stats;
        record.opts = opts;
        record.value = value;
        record.matches = m_matches;
        m_history.add(std::move(record));
    }

    auto pruneEmptySwaths() -> void {
        auto [eraseBegin, eraseEnd] =
            std::ranges::remove_if(m_matches.swaths, [](const auto& swath) {
                return std::ranges::all_of(swath.data, [](const auto& cell) {
                    return cell.matchInfo == MatchFlags::EMPTY;
                });
            });
        m_matches.swaths.erase(eraseBegin, eraseEnd);
    }
};

}  // namespace core
