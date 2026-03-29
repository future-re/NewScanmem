/**
 * @file scan_service.cppm
 * @brief Explicit application service for scan execution
 */

module;

#include <cstddef>
#include <expected>
#include <optional>
#include <string>

export module app.scan_service;

import core.maps;
import core.scanner;
import scan.types;
import value.core;

export namespace app {

enum class ScanExecutionMode {
    SNAPSHOT,
    FILTER,
    RESCAN
};

struct ScanExecutionRequest {
    core::Scanner* scanner{nullptr};
    ScanOptions options{};
    std::optional<UserValue> userValue;
    ScanExecutionMode mode{ScanExecutionMode::SNAPSHOT};
    bool saveToHistory{true};
};

struct ScanExecutionResult {
    ScanStats stats{};
    std::size_t matchCount{0};
    bool isFiltered{false};
};

class ScanService {
   public:
    [[nodiscard]] static auto execute(const ScanExecutionRequest& request)
        -> std::expected<ScanExecutionResult, std::string> {
        if (request.scanner == nullptr) {
            return std::unexpected("Failed to initialize scanner");
        }

        switch (request.mode) {
            case ScanExecutionMode::SNAPSHOT:
                return snapshot(request.scanner, request.options,
                                request.userValue, request.saveToHistory);
            case ScanExecutionMode::FILTER:
                return filter(request.scanner, request.options,
                              request.userValue, request.saveToHistory);
            case ScanExecutionMode::RESCAN:
                return rescan(request.scanner, request.options, request.userValue,
                              request.saveToHistory);
        }
        return std::unexpected("Unknown scan execution mode");
    }

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

    [[nodiscard]] static auto filter(core::Scanner* scanner,
                                     const ScanOptions& opts,
                                     const std::optional<UserValue>& value = std::nullopt,
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

    [[nodiscard]] static auto rescan(core::Scanner* scanner,
                                     const ScanOptions& opts,
                                     const std::optional<UserValue>& value = std::nullopt,
                                     bool saveToHistory = true)
        -> std::expected<ScanExecutionResult, std::string> {
        if (!scanner) {
            return std::unexpected("Scanner is null");
        }
        auto response = scanner->rescan(opts, value, saveToHistory);
        if (!response.success) {
            return std::unexpected(response.error.value_or("Rescan failed"));
        }
        return ScanExecutionResult{.stats = response.stats,
                                   .matchCount = response.matchCount,
                                   .isFiltered = false};
    }
};

}  // namespace app
