#pragma once

/**
 * @file scan_service.hpp
 * @brief Explicit application service for scan execution
 */

#include <cstddef>
#include <expected>
#include <optional>
#include <string>

#include "newscanmem/core/maps.hpp"
#include "newscanmem/core/scanner.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"

namespace app {

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
        -> std::expected<ScanExecutionResult, std::string>;

    [[nodiscard]] static auto snapshot(core::Scanner* scanner,
                                       const ScanOptions& opts,
                                       const std::optional<UserValue>& value = std::nullopt,
                                       bool saveToHistory = true)
        -> std::expected<ScanExecutionResult, std::string>;

    [[nodiscard]] static auto filter(core::Scanner* scanner,
                                     const ScanOptions& opts,
                                     const std::optional<UserValue>& value = std::nullopt,
                                     bool saveToHistory = true)
        -> std::expected<ScanExecutionResult, std::string>;

    [[nodiscard]] static auto rescan(core::Scanner* scanner,
                                     const ScanOptions& opts,
                                     const std::optional<UserValue>& value = std::nullopt,
                                     bool saveToHistory = true)
        -> std::expected<ScanExecutionResult, std::string>;
};

}  // namespace app
