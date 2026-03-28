/**
 * @file scan_history.cppm
 * @brief Management of scan result history (扫描历史管理)
 */

module;

#include <cstddef>
#include <deque>
#include <utility>

export module core.scan_history;

import scan.engine; // for ScanResult

export namespace core {

/**
 * @class ScanHistory
 * @brief Manages a fixed-size history of scan results
 */
class ScanHistory {
   public:
    ScanHistory() = default;

    /**
     * @brief Add a new scan result to history
     * @param result Result to add (will be moved)
     *
     * If history exceeds MAX_HISTORY (10), the oldest result is removed.
     */
    void add(ScanResult result) {
        if (m_results.size() >= MAX_HISTORY) {
            m_results.pop_front();
        }
        m_results.push_back(std::move(result));
    }

    /**
     * @brief Get number of results in history
     * @return Result count
     */
    [[nodiscard]] auto count() const -> std::size_t { return m_results.size(); }

    /**
     * @brief Get a specific scan result by index
     * @param index Index from 0 (oldest) to count()-1 (newest)
     * @return Pointer to result, or nullptr if out of range
     */
    [[nodiscard]] auto get(std::size_t index) const -> const ScanResult* {
        if (index >= m_results.size()) {
            return nullptr;
        }
        return &m_results[index];
    }

    /**
     * @brief Get all scan results
     * @return Reference to deque of results
     */
    [[nodiscard]] auto getAll() const -> const std::deque<ScanResult>& {
        return m_results;
    }

    /**
     * @brief Clear all history
     */
    void clear() { m_results.clear(); }

   private:
    static constexpr std::size_t MAX_HISTORY = 10;
    std::deque<ScanResult> m_results;
};

}  // namespace core
