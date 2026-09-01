#include "memseek/core/scan_history.hpp"

namespace core {
void ScanHistory::add(ScanRecord result) {
    if (m_results.size() >= MAX_HISTORY) m_results.pop_front();
    m_results.push_back(std::move(result));
}
auto ScanHistory::count() const -> std::size_t { return m_results.size(); }
auto ScanHistory::get(const std::size_t index) const -> const ScanRecord* {
    return index < m_results.size() ? &m_results[index] : nullptr;
}
auto ScanHistory::getAll() const -> const std::deque<ScanRecord>& {
    return m_results;
}
void ScanHistory::clear() { m_results.clear(); }
}  // namespace core
