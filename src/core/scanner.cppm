/**
 * @file scanner.cppm
 * @brief High-level scanner interface wrapping scan.engine
 */

module;

#include <unistd.h>

#include <cstddef>
#include <expected>
#include <string>

export module core.scanner;

import scan.engine;    // ScanOptions, ScanStats, runScan
import scan.types;     // ScanDataType, ScanMatchType
import core.targetmem; // MatchesAndOldValuesArray
import value;          // UserValue
import core.maps;      // RegionScanLevel

export namespace core {

/**
 * @class Scanner
 * @brief High-level scanner managing scan sessions
 */
class Scanner {
   public:
    /**
     * @brief Construct scanner for given process
     * @param pid Target process ID
     */
    explicit Scanner(pid_t pid) : m_pid(pid), m_matches(0) {}

    /**
     * @brief Perform memory scan
     * @param opts Scan options
     * @param value Value to search for (nullptr for any)
     * @return Expected scan statistics or error message
     */
    [[nodiscard]] auto performScan(const ScanOptions& opts,
                                   const UserValue* value = nullptr)
        -> std::expected<ScanStats, std::string> {
        auto result = runScan(m_pid, opts, value, m_matches);
        if (!result) {
            return std::unexpected(result.error());
        }

        m_lastStats = *result;
        return m_lastStats;
    }

    /**
     * @brief Get matches from last scan
     * @return Reference to matches array
     */
    [[nodiscard]] auto getMatches() const -> const MatchesAndOldValuesArray& {
        return m_matches;
    }

    /**
     * @brief Get mutable matches array
     * @return Reference to matches array
     */
    [[nodiscard]] auto getMatchesMut() -> MatchesAndOldValuesArray& {
        return m_matches;
    }

    /**
     * @brief Get statistics from last scan
     * @return Last scan statistics
     */
    [[nodiscard]] auto getLastStats() const -> const ScanStats& {
        return m_lastStats;
    }

    /**
     * @brief Clear all matches
     */
    auto clearMatches() -> void { m_matches.swaths.clear(); }

    /**
     * @brief Get number of matches
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
     * @brief Check if scanner has matches
     * @return True if matches exist
     */
    [[nodiscard]] auto hasMatches() const -> bool {
        return getMatchCount() > 0;
    }

    /**
     * @brief Get target PID
     * @return Process ID
     */
    [[nodiscard]] auto getPid() const -> pid_t { return m_pid; }

   private:
    pid_t m_pid;
    MatchesAndOldValuesArray m_matches;
    ScanStats m_lastStats{};
};

/**
 * @brief Create default scan options
 * @return Default scan options
 */
[[nodiscard]] inline auto getDefaultScanOptions() -> ScanOptions {
    return ScanOptions{};
}

/**
 * @brief Create scan options for numeric search
 * @param dataType Data type to search
 * @param matchType Match type
 * @return Configured scan options
 */
[[nodiscard]] inline auto makeNumericScanOptions(
    ScanDataType dataType,
    ScanMatchType matchType = ScanMatchType::MATCHEQUALTO) -> ScanOptions {
    ScanOptions opts;
    opts.dataType = dataType;
    opts.matchType = matchType;
    return opts;
}

/**
 * @brief Create scan options for string search
 * @param matchType Match type
 * @return Configured scan options
 */
[[nodiscard]] inline auto makeStringScanOptions(
    ScanMatchType matchType = ScanMatchType::MATCHEQUALTO) -> ScanOptions {
    ScanOptions opts;
    opts.dataType = ScanDataType::STRING;
    opts.matchType = matchType;
    opts.step = 1;  // String search typically uses step=1
    return opts;
}

}  // namespace core
