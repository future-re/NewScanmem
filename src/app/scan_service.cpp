#include "memseek/app/scan_service.hpp"

namespace app {
namespace {
auto resultFromResponse(const core::ScannerResult& response,
                        const std::string_view fallback, const bool filtered)
    -> std::expected<ScanExecutionResult, std::string> {
    if (!response.success)
        return std::unexpected(response.error.value_or(std::string(fallback)));
    return ScanExecutionResult{.stats = response.stats,
                               .matchCount = response.matchCount,
                               .isFiltered = filtered};
}
}  // namespace
auto ScanService::execute(const ScanExecutionRequest& request)
    -> std::expected<ScanExecutionResult, std::string> {
    if (request.scanner == nullptr)
        return std::unexpected("Failed to initialize scanner");
    switch (request.mode) {
        case ScanExecutionMode::SNAPSHOT:
            return snapshot(request.scanner, request.options, request.userValue,
                            request.saveToHistory);
        case ScanExecutionMode::FILTER:
            return filter(request.scanner, request.options, request.userValue,
                          request.saveToHistory);
        case ScanExecutionMode::RESCAN:
            return rescan(request.scanner, request.options, request.userValue,
                          request.saveToHistory);
    }
    return std::unexpected("Unknown scan execution mode");
}
auto ScanService::snapshot(core::Scanner* scanner, const ScanOptions& options,
                           const std::optional<UserValue>& value,
                           const bool save)
    -> std::expected<ScanExecutionResult, std::string> {
    if (!scanner) return std::unexpected("Scanner is null");
    return resultFromResponse(scanner->snapshot(options, value, save),
                              "Snapshot failed", false);
}
auto ScanService::filter(core::Scanner* scanner, const ScanOptions& options,
                         const std::optional<UserValue>& value, const bool save)
    -> std::expected<ScanExecutionResult, std::string> {
    if (!scanner) return std::unexpected("Scanner is null");
    return resultFromResponse(scanner->filter(options, value, save),
                              "Filter failed", true);
}
auto ScanService::rescan(core::Scanner* scanner, const ScanOptions& options,
                         const std::optional<UserValue>& value, const bool save)
    -> std::expected<ScanExecutionResult, std::string> {
    if (!scanner) return std::unexpected("Scanner is null");
    return resultFromResponse(scanner->rescan(options, value, save),
                              "Rescan failed", false);
}
}  // namespace app
