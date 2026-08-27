#pragma once

/**
 * @file memory_writer.hpp
 * @brief Memory writing abstraction using process_vm_writev
 */

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "newscanmem/core/proc_mem.hpp"
#include "newscanmem/core/scanner.hpp"
#include "newscanmem/scan/match_storage.hpp"
#include "newscanmem/utils/endianness.hpp"
#include "newscanmem/utils/logging.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

namespace core {

// Endianness type and helpers come from utils.endianness

/**
 * @struct WriteResult
 * @brief Result of a single memory write operation
 */
struct WriteResult {
    std::uintptr_t address;    // Base address written to
    std::size_t bytesWritten;  // Number of bytes written
    bool success;              // Whether the write was successful
};

/**
 * @struct VecWriteResult
 * @brief Result of vector write operations
 */
struct VecWriteResult {
    std::size_t successCount;          // Number of successful writes
    std::size_t failedCount;           // Number of failed writes
    std::vector<WriteResult> results;  // Vector of individual write results
    std::vector<std::string>
        errors;  // Vector of error messages for failed writes
};

/**
 * @class MemoryWriter
 * @brief Handles memory writes to target process
 */
class MemoryWriter {
   public:
    explicit MemoryWriter(
        pid_t pid,
        utils::Endianness mode = (std::endian::native == std::endian::little
                                      ? utils::Endianness::LITTLE
                                      : utils::Endianness::BIG));

    auto setEndianness(utils::Endianness mode) -> void;
    [[nodiscard]] auto getEndianness() const noexcept -> utils::Endianness;

    /**
     * @brief Write value to a single matched address
     * @param scanner Scanner instance with matches
     * @param value Value to write
     * @param matchIndex  Vector: Index of the match to write to
     * @return Expected WriteResult or error string
     */
    [[nodiscard]] auto writeToMatch(const Scanner& scanner,
                                    const UserValue& value,
                                    std::vector<size_t> matchIndex)
        -> std::expected<VecWriteResult, std::string>;

   private:
    [[nodiscard]] auto encodeValueBytes(const UserValue& value) const
        -> std::vector<std::uint8_t>;

    [[nodiscard]] static auto resolveMatchAddress(
        const scan::MatchesAndOldValuesArray& matches,
        std::size_t match_index) -> std::expected<std::uintptr_t, std::string>;

    pid_t m_pid;
    utils::Endianness m_endianness{(std::endian::native == std::endian::little
                                        ? utils::Endianness::LITTLE
                                        : utils::Endianness::BIG)};
};

}  // namespace core
