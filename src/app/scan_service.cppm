/**
 * @file scan_service.cppm
 * @brief Thin application service for scan execution
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
import value;

export namespace app {

struct ScanRequest {
    core::Scanner* scanner{nullptr};
    core::RegionScanLevel regionLevel{core::RegionScanLevel::ALL_RW};
    ScanDataType dataType{ScanDataType::ANY_NUMBER};
    ScanMatchType matchType{ScanMatchType::MATCH_ANY};
    std::optional<UserValue> userValue;
    bool saveToHistory{true};
};

struct ScanExecutionResult {
    ScanStats stats{};
    std::size_t matchCount{0};
};

class ScanService {
   public:
    [[nodiscard]] static auto run(const ScanRequest& request)
        -> std::expected<ScanExecutionResult, std::string> {
        if (request.scanner == nullptr) {
            return std::unexpected("Failed to initialize scanner");
        }

        ScanOptions options;
        options.dataType = request.dataType;
        options.matchType = request.matchType;
        options.regionLevel = request.regionLevel;

        auto result = request.userValue.has_value()
                          ? request.scanner->scan(options, *request.userValue,
                                                  request.saveToHistory)
                          : request.scanner->scan(options,
                                                  request.saveToHistory);
        if (!result) {
            return std::unexpected(result.error());
        }

        return ScanExecutionResult{
            .stats = *result, .matchCount = request.scanner->getMatchCount()};
    }
};

}  // namespace app
