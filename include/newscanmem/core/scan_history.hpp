#pragma once

/**
 * @file scan_history.hpp
 * @brief Management of scan result history (扫描历史管理)
 */

#include <cstddef>
#include <deque>
#include <utility>

#include "newscanmem/scan/types.hpp"

namespace core {

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
    void add(ScanRecord result);

    /**
     * @brief Get number of results in history
     * @return Result count
     */
    [[nodiscard]] auto count() const -> std::size_t;

    /**
     * @brief Get a specific scan result by index
     * @param index Index from 0 (oldest) to count()-1 (newest)
     * @return Pointer to result, or nullptr if out of range
     */
    [[nodiscard]] auto get(std::size_t index) const -> const ScanRecord*;

    /**
     * @brief Get all scan results
     * @return Reference to deque of results
     */
    [[nodiscard]] auto getAll() const -> const std::deque<ScanRecord>&;

    /**
     * @brief Clear all history
     */
    void clear();

   private:
    static constexpr std::size_t MAX_HISTORY = 10;
    std::deque<ScanRecord> m_results;
};

}  // namespace core
