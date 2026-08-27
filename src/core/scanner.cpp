#include "newscanmem/core/scanner.hpp"

namespace core {
Scanner::Scanner(const pid_t pid) : m_pid(pid) {}
auto Scanner::snapshot(const ScanOptions& options,
                       const std::optional<UserValue>& value,
                       const bool save) -> ScannerResult {
    clearMatches();
    return doScan(options, value, save);
}
auto Scanner::filter(const ScanOptions& options,
                     const std::optional<UserValue>& value,
                     const bool save) -> ScannerResult {
    if (!hasMatches())
        return {
            .stats = {},
            .matchCount = 0,
            .success = false,
            .error = "No existing matches to filter. Run snapshot() first."};
    const auto stats =
        filterMatches(m_pid, options, value ? &*value : nullptr, m_matches);
    if (!stats)
        return {.stats = {},
                .matchCount = 0,
                .success = false,
                .error = stats.error()};
    pruneEmptySwaths();
    if (save) saveResultToHistory(*stats, options, value);
    return {.stats = *stats, .matchCount = getMatchCount(), .success = true};
}
auto Scanner::rescan(const ScanOptions& options,
                     const std::optional<UserValue>& value,
                     const bool save) -> ScannerResult {
    reset();
    return doScan(options, value, save);
}
auto Scanner::getResultCount() const -> std::size_t {
    return m_history.count();
}
auto Scanner::getResult(const std::size_t index) const -> const ScanRecord* {
    return m_history.get(index);
}
auto Scanner::getResults() const -> const std::deque<ScanRecord>& {
    return m_history.getAll();
}
void Scanner::clearResultHistory() { m_history.clear(); }
auto Scanner::getMatches() const -> const scan::MatchesAndOldValuesArray& {
    return m_matches;
}
auto Scanner::getMatches() -> scan::MatchesAndOldValuesArray& {
    return m_matches;
}
void Scanner::clearMatches() { m_matches.swaths.clear(); }
void Scanner::reset() {
    clearMatches();
    m_history.clear();
}
auto Scanner::getMatchCount() const -> std::size_t {
    std::size_t count = 0;
    for (const auto& swath : m_matches.swaths)
        for (const auto& element : swath.data)
            if (element.matchInfo != MatchFlags::EMPTY) ++count;
    return count;
}
auto Scanner::hasMatches() const -> bool { return getMatchCount() != 0; }
auto Scanner::getPid() const -> pid_t { return m_pid; }
auto Scanner::getLastDataType() const -> std::optional<ScanDataType> {
    return m_lastDataType;
}
auto Scanner::doScan(const ScanOptions& options,
                     const std::optional<UserValue>& value,
                     const bool save) -> ScannerResult {
    m_lastDataType = options.dataType;
    const auto result = runScanParallel(
        m_pid, options, value ? &*value : nullptr, m_matches, nullptr);
    if (!result)
        return {.stats = {},
                .matchCount = 0,
                .success = false,
                .error = result.error()};
    if (save) saveResultToHistory(*result, options, value);
    return {.stats = *result, .matchCount = getMatchCount(), .success = true};
}
void Scanner::saveResultToHistory(const ScanStats& stats,
                                  const ScanOptions& options,
                                  const std::optional<UserValue>& value) {
    m_history.add({.stats = stats,
                   .matches = m_matches,
                   .opts = options,
                   .value = value});
}
void Scanner::pruneEmptySwaths() {
    const auto [first, last] =
        std::ranges::remove_if(m_matches.swaths, [](const auto& swath) {
            return std::ranges::all_of(swath.data, [](const auto& cell) {
                return cell.matchInfo == MatchFlags::EMPTY;
            });
        });
    m_matches.swaths.erase(first, last);
}
}  // namespace core
