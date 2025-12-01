/**
 * @file scanner.cppm
 * @brief High-level scanner interface wrapping scan.engine
 */

module;

#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <vector>

export module core.scanner;

import scan.engine;    // ScanOptions, ScanStats, runScan, ProcMemReader
import scan.types;     // ScanDataType, ScanMatchType
import scan.factory;   // smGetScanroutine
import core.targetmem; // MatchesAndOldValuesArray
import value;          // UserValue
import core.maps;      // RegionScanLevel

/**
 * @struct ScanResult
 * @brief Stores the complete result of a single scan operation
 */
export struct ScanResult {
    ScanStats stats;                   // Statistics from this scan
    MatchesAndOldValuesArray matches;  // Matches from this scan
    ScanOptions opts;                  // Options used for this scan
    std::optional<UserValue> value;    // User value used (if any)
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
export class Scanner {
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
        // Full scan mode: clear previous active matches
        m_matches.swaths.clear();
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
            m_results.push_back(std::move(scanResult));
        }

        return *result;
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
        auto routine = smGetScanroutine(
            opts.dataType, opts.matchType,
            (value != nullptr) ? value->flags : MatchFlags::EMPTY,
            opts.reverseEndianness);
        if (!routine) {
            return std::unexpected("no scan routine for filtered options");
        }
        ProcMemReader reader{m_pid};
        if (auto err = reader.open(); !err) {
            return std::unexpected(err.error());
        }
        const std::size_t SLICE_SIZE = bytesNeededForType(opts.dataType);
        std::vector<std::uint8_t> buffer(SLICE_SIZE);
        ScanStats stats{};
        for (auto& swath : m_matches.swaths) {
            narrowSwath(swath, routine, value, reader, buffer, stats);
        }
        pruneEmptySwaths();
        if (saveToHistory) {
            ScanResult scanRecord;
            scanRecord.stats = stats;
            scanRecord.opts = opts;
            if (value != nullptr) {
                scanRecord.value = *value;
            }
            scanRecord.matches = m_matches;
            m_results.push_back(std::move(scanRecord));
        }
        return stats;
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
    [[nodiscard]] auto getMatches() const -> const MatchesAndOldValuesArray& {
        return m_matches;
    }

    /**
     * @brief Get mutable current/active matches
     * @return Reference to matches array
     */
    [[nodiscard]] auto getMatches() -> MatchesAndOldValuesArray& {
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

   private:
    pid_t m_pid;
    MatchesAndOldValuesArray m_matches;  // Current/active matches
    std::deque<ScanResult> m_results;  // History of scan results (stable refs)
    static auto bytesNeededForType(ScanDataType dataType) -> std::size_t {
        switch (dataType) {
            case ScanDataType::INTEGER8:
                return 1;
            case ScanDataType::INTEGER16:
                return 2;
            case ScanDataType::INTEGER32:
            case ScanDataType::FLOAT32:
                return 4;
            case ScanDataType::INTEGER64:
            case ScanDataType::FLOAT64:
                return 8;
            case ScanDataType::STRING:
            case ScanDataType::BYTEARRAY:
                return 32;
            case ScanDataType::ANYINTEGER:
                return 8;
            case ScanDataType::ANYFLOAT:
                return 8;
            case ScanDataType::ANYNUMBER:
                return 8;
        }
        return 8;
    }
    static auto narrowSwath(MatchesAndOldValuesSwath& swath, auto& routine,
                            const UserValue* value, ProcMemReader& reader,
                            std::vector<std::uint8_t>& buffer, ScanStats& stats)
        -> void {
        auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
        for (std::size_t i = 0; i < swath.data.size(); ++i) {
            auto& cell = swath.data[i];
            if (cell.matchInfo == MatchFlags::EMPTY) {
                continue;
            }
            void* addr = static_cast<void*>(base + i);
            auto readExp = reader.read(addr, buffer.data(), buffer.size());
            if (!readExp || *readExp == 0) {
                cell.matchInfo = MatchFlags::EMPTY;
                continue;
            }
            Mem64 mem{buffer.data(), *readExp};
            MatchFlags newFlags = MatchFlags::EMPTY;
            const Value* oldValuePtr = nullptr;
            const unsigned MATCHED_LEN =
                routine(&mem, *readExp, oldValuePtr, value, &newFlags);
            if (MATCHED_LEN > 0) {
                cell.matchInfo = newFlags;
                stats.matches++;
            } else {
                cell.matchInfo = MatchFlags::EMPTY;
            }
            stats.bytesScanned += *readExp;
        }
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
