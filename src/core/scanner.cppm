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
    explicit Scanner(pid_t pid) : pid_(pid), matches_(0) {}

    /**
     * @brief Perform memory scan
     * @param opts Scan options
     * @param value Value to search for (nullptr for any)
     * @return Expected scan statistics or error message
     */
    [[nodiscard]] auto performScan(const ScanOptions& opts,
                                   const UserValue* value = nullptr)
        -> std::expected<ScanStats, std::string> {
        auto result = runScan(pid_, opts, value, matches_);
        if (!result) {
            return std::unexpected(result.error());
        }

        lastStats_ = *result;
        return lastStats_;
    }

    /**
     * @brief Get matches from last scan
     * @return Reference to matches array
     */
    [[nodiscard]] auto getMatches() const -> const MatchesAndOldValuesArray& {
        return matches_;
    }

    /**
     * @brief Get mutable matches array
     * @return Reference to matches array
     */
    [[nodiscard]] auto getMatchesMut() -> MatchesAndOldValuesArray& {
        return matches_;
    }

    /**
     * @brief Get statistics from last scan
     * @return Last scan statistics
     */
    [[nodiscard]] auto getLastStats() const -> const ScanStats& {
        return lastStats_;
    }

    /**
     * @brief Clear all matches
     */
    auto clearMatches() -> void { matches_.swaths.clear(); }

    /**
     * @brief Get number of matches
     * @return Match count
     */
    [[nodiscard]] auto getMatchCount() const -> std::size_t {
        std::size_t count = 0;
        for (const auto& swath : matches_.swaths) {
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
    [[nodiscard]] auto getPid() const -> pid_t { return pid_; }

   private:
    pid_t pid_;
    MatchesAndOldValuesArray matches_;
    ScanStats lastStats_{};
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
