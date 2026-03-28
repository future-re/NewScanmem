/**
 * @file scan_service.cppm
 * @brief Application service for scan execution with modern API
 */

module;

#include <cstddef>
#include <expected>
#include <optional>
#include <string>

export module app.scan_service;

import core.maps;
import core.scanner;
import scan.engine;
import scan.types;
import value.core;

export namespace app {

/**
 * @brief Scan mode for high-level API
 */
enum class ScanServiceMode {
    AUTO,       ///< Auto-detect: snapshot if no matches, filter if has matches
    SNAPSHOT,   ///< Force full memory scan
    FILTER,     ///< Force filter on existing matches
    RESCAN      ///< Force reset and full scan
};

/**
 * @struct ScanRequest
 * @brief Request for scan service
 */
struct ScanRequest {
    core::Scanner* scanner{nullptr};
    core::RegionScanLevel regionLevel{core::RegionScanLevel::ALL_RW};
    ScanDataType dataType{ScanDataType::ANY_NUMBER};
    ScanMatchType matchType{ScanMatchType::MATCH_ANY};
    std::optional<UserValue> userValue;
    ScanServiceMode mode{ScanServiceMode::AUTO};
    bool saveToHistory{true};
};

/**
 * @struct ScanExecutionResult
 * @brief Result of scan execution
 */
struct ScanExecutionResult {
    ScanStats stats{};
    std::size_t matchCount{0};
    bool isFiltered{false};  ///< True if this was a filter operation
};

/**
 * @class ScanService
 * @brief High-level service for scan operations
 *
 * Provides a simplified interface over core::Scanner with automatic
 * mode selection and error handling.
 */
class ScanService {
   public:
    /**
     * @brief Execute a scan based on the request
     * @param request Scan request parameters
     * @return Expected result or error message
     */
    [[nodiscard]] static auto run(const ScanRequest& request)
        -> std::expected<ScanExecutionResult, std::string> {
        if (request.scanner == nullptr) {
            return std::unexpected("Failed to initialize scanner");
        }

        ScanOptions options;
        options.dataType = request.dataType;
        options.matchType = request.matchType;
        options.regionLevel = request.regionLevel;

        // Determine scan mode
        core::ScanMode mode = core::ScanMode::FULL_SCAN;
        bool isFiltered = false;

        switch (request.mode) {
            case ScanServiceMode::AUTO:
                if (request.scanner->hasMatches()) {
                    mode = core::ScanMode::INCREMENTAL;
                    isFiltered = true;
                } else {
                    mode = core::ScanMode::FULL_SCAN;
                }
                break;
            case ScanServiceMode::SNAPSHOT:
                mode = core::ScanMode::FULL_SCAN;
                break;
            case ScanServiceMode::FILTER:
                mode = core::ScanMode::INCREMENTAL;
                isFiltered = true;
                if (!request.userValue.has_value()) {
                    return std::unexpected("Filter mode requires a user value");
                }
                break;
            case ScanServiceMode::RESCAN:
                mode = core::ScanMode::RESCAN;
                break;
        }

        // Build scan request
        core::ScanRequest scanReq{
            .mode = mode,
            .options = options,
            .targetValue = request.userValue,
            .saveToHistory = request.saveToHistory
        };

        auto response = request.scanner->scan(scanReq);
        if (!response.success) {
            return std::unexpected(response.error.value_or("Scan failed"));
        }

        return ScanExecutionResult{
            .stats = response.stats,
            .matchCount = response.matchCount,
            .isFiltered = isFiltered
        };
    }

    /**
     * @brief Perform a snapshot scan (full memory scan)
     */
    [[nodiscard]] static auto snapshot(core::Scanner* scanner,
                                       const ScanOptions& opts,
                                       const std::optional<UserValue>& value = std::nullopt,
                                       bool saveToHistory = true)
        -> std::expected<ScanExecutionResult, std::string> {
        if (!scanner) {
            return std::unexpected("Scanner is null");
        }
        auto response = scanner->snapshot(opts, value, saveToHistory);
        if (!response.success) {
            return std::unexpected(response.error.value_or("Snapshot failed"));
        }
        return ScanExecutionResult{.stats = response.stats,
                                   .matchCount = response.matchCount,
                                   .isFiltered = false};
    }

    /**
     * @brief Perform a filter scan (incremental on existing matches)
     */
    [[nodiscard]] static auto filter(core::Scanner* scanner,
                                     const ScanOptions& opts,
                                     const UserValue& value,
                                     bool saveToHistory = true)
        -> std::expected<ScanExecutionResult, std::string> {
        if (!scanner) {
            return std::unexpected("Scanner is null");
        }
        auto response = scanner->filter(opts, value, saveToHistory);
        if (!response.success) {
            return std::unexpected(response.error.value_or("Filter failed"));
        }
        return ScanExecutionResult{.stats = response.stats,
                                   .matchCount = response.matchCount,
                                   .isFiltered = true};
    }
};

}  // namespace app
