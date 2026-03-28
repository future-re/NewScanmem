/**
 * @file scanner.cppm
 * @brief High-level scanner interface with explicit operations
 *
 * Replaces the old ambiguous scan() method with explicit operations:
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

import scan.engine;        // runScan, ScanOptions, ScanStats
import scan.co_engine;     // runScanParallel
import scan.types;         // ScanDataType, ScanMatchType
import scan.filter;        // filterMatches
import scan.match_storage; // MatchesAndOldValuesArray
import value.core;         // UserValue, Value
import value.flags;
import core.maps;          // RegionScanLevel
import core.scan_history;  // ScanHistory
import utils.logging;      // Logger

export namespace core {

/**
 * @enum ScanMode
 * @brief Explicit scan operation modes
 */
enum class ScanMode {
    FULL_SCAN,    ///< Full memory scan, clears existing matches
    INCREMENTAL,  ///< Filter on existing matches only
    RESCAN        ///< Clear matches then full scan
};

/**
 * @struct ScanRequest
 * @brief Request for a scan operation
 */
struct ScanRequest {
    ScanMode mode{ScanMode::FULL_SCAN};  ///< Scan operation mode
    ScanOptions options;                  ///< Scan options (data type, match type, etc.)
    std::optional<UserValue> targetValue; ///< Target value (for comparison matches)
    bool saveToHistory{false};            ///< Whether to save result to history
};

/**
 * @struct ScanResponse
 * @brief Response from a scan operation
 */
struct ScanResponse {
    ScanStats stats;                      ///< Scan statistics
    std::size_t matchCount{0};            ///< Total matches found
    bool success{false};                  ///< Whether scan succeeded
    std::optional<std::string> error;     ///< Error message if failed
};

/**
 * @class Scanner
 * @brief High-level scanner with explicit operations
 *
 * Design principles:
 * 1. Explicit operations: No magic behavior, caller chooses the operation
 * 2. Clear state management: Matches, history, and state are well-defined
 * 3. Type-safe: Uses modern C++ features for safety
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
    [[nodiscard]] auto snapshot(const ScanOptions& opts,
                                const std::optional<UserValue>& value = std::nullopt,
                                bool saveToHistory = false) -> ScanResponse {
        clearMatches();
        return doScan(opts, value, saveToHistory);
    }

    /**
     * @brief Filter existing matches incrementally
     * @param opts Scan options
     * @param value Target value (required for filtering)
     * @param saveToHistory Whether to save to history
     * @return Scan response with results
     */
    [[nodiscard]] auto filter(const ScanOptions& opts,
                              const UserValue& value,
                              bool saveToHistory = false) -> ScanResponse {
        if (!hasMatches()) {
            return ScanResponse{.stats{}, .matchCount{0}, .success{false},
                                .error{"No existing matches to filter. Run snapshot() first."}};
        }

        auto statsExp = filterMatches(m_pid, opts, &value, m_matches);
        if (!statsExp) {
            return ScanResponse{.stats{}, .matchCount{0}, .success{false}, .error{statsExp.error()}};
        }

        pruneEmptySwaths();

        if (saveToHistory) {
            saveResultToHistory(*statsExp, opts, value);
        }

        return ScanResponse{.stats{*statsExp}, .matchCount{getMatchCount()}, .success{true}};
    }

    /**
     * @brief Rescan: clear matches and perform full scan
     * @param opts Scan options
     * @param value Optional target value
     * @param saveToHistory Whether to save to history
     * @return Scan response with results
     */
    [[nodiscard]] auto rescan(const ScanOptions& opts,
                              const std::optional<UserValue>& value = std::nullopt,
                              bool saveToHistory = false) -> ScanResponse {
        reset();
        return doScan(opts, value, saveToHistory);
    }

    /**
     * @brief Unified scan interface (for backward compatibility)
     * @param request Scan request with mode and options
     * @return Scan response
     */
    [[nodiscard]] auto scan(const ScanRequest& request) -> ScanResponse {
        switch (request.mode) {
            case ScanMode::FULL_SCAN:
                return snapshot(request.options, request.targetValue, request.saveToHistory);
            case ScanMode::INCREMENTAL:
                if (!request.targetValue) {
                    return ScanResponse{.success{false},
                                        .error{"INCREMENTAL mode requires a target value"}};
                }
                return filter(request.options, *request.targetValue, request.saveToHistory);
            case ScanMode::RESCAN:
                return rescan(request.options, request.targetValue, request.saveToHistory);
        }
        return ScanResponse{.success{false}, .error{"Unknown scan mode"}};
    }

    // ====================================================================
    // Legacy API (deprecated but functional)
    // ====================================================================

    /**
     * @brief Legacy perform scan (full scan)
     * @deprecated Use snapshot() explicitly
     */
    [[nodiscard]] auto performScan(const ScanOptions& opts,
                                   const std::optional<UserValue>& value = std::nullopt,
                                   bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        auto result = snapshot(opts, value, saveToHistory);
        if (!result.success) {
            return std::unexpected{result.error.value_or("Scan failed")};
        }
        return result.stats;
    }

    /**
     * @brief Legacy perform filtered scan
     * @deprecated Use filter() explicitly
     */
    [[nodiscard]] auto performFilteredScan(const ScanOptions& opts,
                                           const std::optional<UserValue>& value = std::nullopt,
                                           bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        if (!hasMatches()) {
            return std::unexpected{"No existing matches to filter; perform full scan first"};
        }
        if (value) {
            auto result = filter(opts, *value, saveToHistory);
            if (!result.success) {
                return std::unexpected{result.error.value_or("Filter failed")};
            }
            return result.stats;
        }
        // Filter without value
        auto statsExp = filterMatches(m_pid, opts, nullptr, m_matches);
        if (!statsExp) {
            return std::unexpected{statsExp.error()};
        }
        pruneEmptySwaths();
        if (saveToHistory) {
            ScanResult scanResult;
            scanResult.stats = *statsExp;
            scanResult.opts = opts;
            scanResult.matches = m_matches;
            m_history.add(std::move(scanResult));
        }
        return *statsExp;
    }

    /**
     * @brief Legacy scan with value (auto-determines full/filter)
     * @deprecated Use snapshot() or filter() explicitly
     */
    [[nodiscard]] auto scan(const ScanOptions& opts, const UserValue& value,
                            bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        if (!hasMatches()) {
            auto result = doScan(opts, value, saveToHistory);
            if (!result.success) {
                return std::unexpected{result.error.value_or("Scan failed")};
            }
            return result.stats;
        }
        auto result = filter(opts, value, saveToHistory);
        if (!result.success) {
            return std::unexpected{result.error.value_or("Filter failed")};
        }
        return result.stats;
    }

    /**
     * @brief Legacy scan without value (auto-determines full/filter)
     * @deprecated Use snapshot() or filter() explicitly
     */
    [[nodiscard]] auto scan(const ScanOptions& opts, bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        if (!hasMatches()) {
            auto result = doScan(opts, std::nullopt, saveToHistory);
            if (!result.success) {
                return std::unexpected{result.error.value_or("Scan failed")};
            }
            return result.stats;
        }
        // Filter without value
        auto statsExp = filterMatches(m_pid, opts, nullptr, m_matches);
        if (!statsExp) {
            return std::unexpected{statsExp.error()};
        }
        pruneEmptySwaths();
        if (saveToHistory) {
            ScanResult scanResult;
            scanResult.stats = *statsExp;
            scanResult.opts = opts;
            m_history.add(std::move(scanResult));
        }
        return *statsExp;
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
    [[nodiscard]] auto getResult(std::size_t index) const -> const ScanResult* {
        return m_history.get(index);
    }

    /**
     * @brief Get all scan results
     */
    [[nodiscard]] auto getResults() const -> const std::deque<ScanResult>& {
        return m_history.getAll();
    }

    /**
     * @brief Clear scan result history
     */
    auto clearResultHistory() -> void { m_history.clear(); }

    /**
     * @brief Get current/active matches from most recent scan
     */
    [[nodiscard]] auto getMatches() const -> const scan::MatchesAndOldValuesArray& {
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

    /**
     * @brief Save current matches to history
     */
    auto saveCurrentToHistory() -> void {
        ScanResult scanResult;
        scanResult.matches = m_matches;
        m_history.add(std::move(scanResult));
    }

   private:
    pid_t m_pid;
    scan::MatchesAndOldValuesArray m_matches;
    ScanHistory m_history;
    std::optional<ScanDataType> m_lastDataType;

    // Internal implementation
    [[nodiscard]] auto doScan(const ScanOptions& opts,
                              const std::optional<UserValue>& value,
                              bool saveToHistory) -> ScanResponse {
        m_lastDataType = opts.dataType;
        auto result = runScanParallel(m_pid, opts, 
                                      value ? &*value : nullptr,
                                      m_matches, nullptr);
        if (!result) {
            return ScanResponse{.stats{}, .matchCount{0}, .success{false}, .error{result.error()}};
        }

        if (saveToHistory && value) {
            saveResultToHistory(*result, opts, *value);
        } else if (saveToHistory) {
            ScanResult scanResult;
            scanResult.stats = *result;
            scanResult.opts = opts;
            scanResult.matches = m_matches;
            m_history.add(std::move(scanResult));
        }

        return ScanResponse{.stats{*result}, .matchCount{getMatchCount()}, .success{true}};
    }

    auto saveResultToHistory(const ScanStats& stats, const ScanOptions& opts,
                             const UserValue& value) -> void {
        ScanResult scanResult;
        scanResult.stats = stats;
        scanResult.opts = opts;
        scanResult.value = value;
        scanResult.matches = m_matches;
        m_history.add(std::move(scanResult));
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
