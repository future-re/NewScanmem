/**
 * @file memory_writer.cppm
 * @brief Memory writing abstraction using process_vm_writev
 */

module;

#include <sys/uio.h>
#include <unistd.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module core.memory_writer;

import value.core;

import core.scanner;
import core.match;
import scan.match_storage;
import utils.endianness;
import value.flags;
import utils.logging;

using scan::OldValueAndMatchInfo;

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
export struct VecWriteResult {
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
export class MemoryWriter {
   public:
    explicit MemoryWriter(
        pid_t pid,
        utils::Endianness mode = (std::endian::native == std::endian::little
                                      ? utils::Endianness::LITTLE
                                      : utils::Endianness::BIG))
        : m_pid(pid), m_endianness(mode) {}

    auto setEndianness(utils::Endianness mode) -> void { m_endianness = mode; }
    [[nodiscard]] auto getEndianness() const noexcept -> utils::Endianness {
        return m_endianness;
    }

    /**
     * @brief Write value to a single matched address
     * @param scanner Scanner instance with matches
     * @param value Value to write
     * @param matchIndex  Vector: Index of the match to write to
     * @return Expected WriteResult or error string
     */
    [[nodiscard]] auto writeToMatch(const Scanner& scanner, UserValue& value,
                                    std::vector<size_t> matchIndex)
        -> std::expected<VecWriteResult, std::string> {
        utils::Logger::instance().debug("UserValue: {}", value);
        utils::Logger::instance().debug("Preparing to write value to match at index {}",
                             matchIndex.empty() ? 0 : matchIndex[0]);
    }

   private:
    pid_t m_pid;
    utils::Endianness m_endianness{(std::endian::native == std::endian::little
                                        ? utils::Endianness::LITTLE
                                        : utils::Endianness::BIG)};
};

}  // namespace core
