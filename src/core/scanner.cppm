/**
 * @file scanner.cppm
 * @brief High-level scanner interface wrapping scan.engine
 */

module;

#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <expected>
#include <functional>
#include <optional>
#include <string>

export module core.scanner;

import scan.engine;        // ScanOptions, ScanStats, runScan
import scan.types;         // ScanDataType, ScanMatchType
import scan.filter;        // filterMatches
import scan.match_storage; // MatchesAndOldValuesArray
import value;              // UserValue
import core.maps;          // RegionScanLevel

export namespace core {

/**
 * @struct ScanResult
 * @brief Stores the complete result of a single scan operation
 */
struct ScanResult {
    ScanStats stats;                         // Statistics from this scan
    scan::MatchesAndOldValuesArray matches;  // Matches from this scan
    ScanOptions opts;                        // Options used for this scan
    std::optional<UserValue> value;          // User value used (if any)
    ScanResult() : stats{}, opts{} {}
};

/**
 * @class Scanner
 * @brief High-level scanner managing scan sessions
 *
 * The Scanner maintains:
 * - m_matches: Current/active matches from the most recent scan
 * - m_results: History queue of all saved scan results
 */
class Scanner {
   public:
    /**
     * @brief Construct scanner for given process
     * @param pid Target process ID
     */
    explicit Scanner(pid_t pid) : m_pid(pid) {}

    /**
     * @brief Perform memory scan
     * @param opts Scan options
     * @param value Value to search for (nullptr for any)
     * @param saveToHistory If true, save scan results to history queue
     * @return Expected scan statistics or error message
     */
    [[nodiscard]] auto performScan(const ScanOptions& opts,
                                   const UserValue* value = nullptr,
                                   bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        // Full scan mode: clear previous active matches (snapshot refresh)
        m_matches.swaths.clear();
        m_lastDataType = opts.dataType;  // Record data type
        auto result = runScan(m_pid, opts, value, m_matches);
        if (!result) {
            return std::unexpected(result.error());
        }

        if (saveToHistory) {
            // Initialize history item; contents are copied from current matches
            ScanResult scanResult;
            scanResult.stats = *result;
            scanResult.opts = opts;
            if (value != nullptr) {
                scanResult.value = *value;
            }
            // Copy current matches to result history
            scanResult.matches = m_matches;
            addToHistory(std::move(scanResult));
        }

        return *result;
    }

    /**
     * @brief Default workflow: first call does full scan; subsequent calls
     *        do filtered scan on existing matches. This matches the common UX
     *        expectation of "初次全量，后续按条件缩小"。
     */
    [[nodiscard]] auto scan(const ScanOptions& opts,
                            const UserValue* value = nullptr,
                            bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        if (!hasMatches()) {
            return performScan(opts, value, saveToHistory);
        }
        return performFilteredScan(opts, value, saveToHistory);
    }

    /**
     * @brief Perform a filtered (incremental) scan only on previously matched
     * addresses Narrows existing matches according to new scan options/value
     * without rescanning all regions. Logic: For every byte previously marked
     * as matched, re-read memory at that address and apply the selected scan
     * routine. Bytes failing the new criterion are cleared. Empty swaths are
     * pruned.
     * @param opts New scan options (dataType/matchType/etc.)
     * @param value New user value (optional)
     * @param saveToHistory If true, snapshot the narrowed result into history
     * @return Expected updated stats or error string
     */
    [[nodiscard]] auto performFilteredScan(const ScanOptions& opts,
                                           const UserValue* value = nullptr,
                                           bool saveToHistory = false)
        -> std::expected<ScanStats, std::string> {
        if (!hasMatches()) {
            return std::unexpected(
                "no existing matches to narrow; perform full scan first");
        }

        auto statsExp = filterMatches(m_pid, opts, value, m_matches);
        if (!statsExp) {
            return std::unexpected(statsExp.error());
        }
        pruneEmptySwaths();
        if (saveToHistory) {
            ScanResult scanRecord;
            scanRecord.stats = *statsExp;
            scanRecord.opts = opts;
            if (value != nullptr) {
                scanRecord.value = *value;
            }
            scanRecord.matches = m_matches;
            addToHistory(std::move(scanRecord));
        }
        return *statsExp;
    }

    /**
     * @brief Get number of saved scan results in history
     * @return Result count
     */
    [[nodiscard]] auto getResultCount() const -> std::size_t {
        return m_results.size();
    }

    /**
     * @brief Get a specific scan result by index
     * @param index Index into results history (0 = oldest)
     * @return Pointer to result, or nullptr if index out of range
     */
    [[nodiscard]] auto getResult(std::size_t index) const
        -> std::optional<std::reference_wrapper<const ScanResult>> {
        if (index >= m_results.size()) {
            return std::nullopt;
        }
        return std::cref(m_results[index]);
    }

    /**
     * @brief Get all scan results
     * @return Reference to results vector
     */
    [[nodiscard]] auto getResults() const -> const std::deque<ScanResult>& {
        return m_results;
    }

    /**
     * @brief Clear scan result history
     */
    auto clearResultHistory() -> void { m_results.clear(); }

    /**
     * @brief Get current/active matches from most recent scan
     * @return Reference to matches array
     */
    [[nodiscard]] auto getMatches() const
        -> const scan::MatchesAndOldValuesArray& {
        return m_matches;
    }

    /**
     * @brief Get mutable current/active matches
     * @return Reference to matches array
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
        m_results.clear();
    }

    /**
     * @brief Get number of matches in current scan
     * @return Match count
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
     * @return True if matches exist
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
     * @return Process ID
     */
    [[nodiscard]] auto getPid() const -> pid_t { return m_pid; }

    /**
     * @brief Get last scan data type
     * @return Optional ScanDataType
     */
    [[nodiscard]] auto getLastDataType() const -> std::optional<ScanDataType> {
        return m_lastDataType;
    }

   private:
    static constexpr std::size_t MAX_HISTORY = 10;
    pid_t m_pid;
    scan::MatchesAndOldValuesArray m_matches;  // Current/active matches
    std::deque<ScanResult> m_results;  // History of scan results (stable refs)
    std::optional<ScanDataType> m_lastDataType;  // Last scan data type
    auto addToHistory(ScanResult&& result) -> void {
        if (m_results.size() >= MAX_HISTORY) {
            m_results.pop_front();
        }
        m_results.push_back(std::move(result));
    }
    auto pruneEmptySwaths() -> void {
        auto eraseIter = std::remove_if(
            m_matches.swaths.begin(), m_matches.swaths.end(),
            [](const auto& swath) {
                bool allEmpty = std::all_of(
                    swath.data.begin(), swath.data.end(), [](const auto& cell) {
                        return cell.matchInfo == MatchFlags::EMPTY;
                    });
                return allEmpty;
            });
        m_matches.swaths.erase(eraseIter, m_matches.swaths.end());
    }
};

}  // namespace core
