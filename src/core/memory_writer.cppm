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

import value;

import core.scanner;
import core.match;
import scan.match_storage;
import utils.endianness;
import value.flags;
import utils.logging;

using scan::OldValueAndMatchInfo;

export namespace core {

// Endianness type and helpers come from utils.endianness

/**
 * @struct WriteResult
 * @brief Result of a single memory write operation
 */
struct WriteResult {
    std::uintptr_t address;
    std::size_t bytesWritten;
    bool success;
};

/**
 * @struct BatchWriteResult
 * @brief Result of batch write operations
 */
struct BatchWriteResult {
    std::size_t successCount;
    std::size_t failedCount;
    std::vector<WriteResult> results;
    std::vector<std::string> errors;
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
     * @param matchIndex Index of the match to write to
     * @return Expected WriteResult or error string
     */
    [[nodiscard]] auto writeToMatch(const Scanner& scanner, UserValue& value,
                                    std::vector<size_t> matchIndex)
        -> std::expected<WriteResult, std::string> {
        utils::Logger::debug("UserValue: {}", value);
        utils::Logger::debug("Preparing to write value to match at index {}",
                             matchIndex.empty() ? 0 : matchIndex[0]);
    }

   private:
    pid_t m_pid;
    utils::Endianness m_endianness{(std::endian::native == std::endian::little
                                        ? utils::Endianness::LITTLE
                                        : utils::Endianness::BIG)};
};

}  // namespace core
