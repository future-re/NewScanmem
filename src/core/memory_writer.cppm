/**
 * @file memory_writer.cppm
 * @brief Memory writing abstraction using process_vm_writev
 */

module;

#include <sys/uio.h>
#include <unistd.h>

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <vector>

export module core.memory_writer;

import core.scanner;
import core.match;
import core.targetmem;
import utils.endianness;
import value.flags;

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
     * @brief Write a value to memory at the specified address
     * @param address Target address in remote process
     * @param value Value to write (will be truncated to writeLen bytes)
     * @param writeLen Number of bytes to write (1, 2, 4, or 8)
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto write(std::uintptr_t address, std::uint64_t value,
                             std::size_t writeLen) const
        -> std::expected<WriteResult, std::string> {
        if (writeLen == 0 || writeLen > 8) {
            return std::unexpected(
                std::format("Invalid write length: {}", writeLen));
        }

        // Prepare buffer in target endianness
        std::array<std::uint8_t, 8> buf{};
        std::uint64_t normalized =
            utils::toTargetEndian<std::uint64_t>(value, m_endianness);
        std::memcpy(buf.data(), &normalized, std::min(writeLen, buf.size()));

        iovec local{.iov_base = buf.data(), .iov_len = writeLen};
        iovec remote{.iov_base = std::bit_cast<void*>(address),
                     .iov_len = writeLen};

        ssize_t written = process_vm_writev(m_pid, &local, 1, &remote, 1, 0);

        if (written == static_cast<ssize_t>(writeLen)) {
            return WriteResult{
                .address = address, .bytesWritten = writeLen, .success = true};
        }

        return std::unexpected(
            std::format("Failed to write {} bytes to 0x{:016x} (wrote: {})",
                        writeLen, address, written));
    }

    /**
     * @brief Write a single byte to memory
     * @param address Target address
     * @param value Byte value
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto writeByte(std::uintptr_t address,
                                 std::uint8_t value) const
        -> std::expected<WriteResult, std::string> {
        return write(address, value, 1);
    }

    /**
     * @brief Write bytes from a buffer to memory
     * @param address Target address
     * @param data Pointer to data buffer
     * @param size Number of bytes to write
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto writeBytes(std::uintptr_t address, const void* data,
                                  std::size_t size)
        -> std::expected<WriteResult, std::string> {
        if (data == nullptr || size == 0) {
            return std::unexpected("Invalid data or size");
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        iovec local{.iov_base = const_cast<void*>(data), .iov_len = size};
        iovec remote{.iov_base = std::bit_cast<void*>(address),
                     .iov_len = size};

        ssize_t written = process_vm_writev(m_pid, &local, 1, &remote, 1, 0);

        if (written == static_cast<ssize_t>(size)) {
            return WriteResult{
                .address = address, .bytesWritten = size, .success = true};
        }

        return std::unexpected(
            std::format("Failed to write {} bytes to 0x{:016x} (wrote: {})",
                        size, address, written));
    }

    /**
     * @brief Write value to all matches in scanner
     * @param scanner Scanner with match results
     * @param value Value to write
     * @return Batch write result
     */
    [[nodiscard]] auto writeToMatches(const Scanner& scanner,
                                      std::uint64_t value) -> BatchWriteResult {
        return writeToMatchesImpl(scanner, value, std::nullopt);
    }

    /**
     * @brief Write value to a specific match by index
     * @param scanner Scanner with match results
     * @param value Value to write
     * @param index Target match index
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto writeToMatch(const Scanner& scanner, std::uint64_t value,
                                    std::size_t index)
        -> std::expected<WriteResult, std::string> {
        auto result = writeToMatchesImpl(scanner, value, index);

        if (result.successCount == 0) {
            if (!result.errors.empty()) {
                return std::unexpected(result.errors[0]);
            }
            return std::unexpected("Match index not found");
        }

        // Return the first (and only) successful write
        return WriteResult{.address = 0,
                           .bytesWritten = 0,
                           .success = true};  // Simplified for now
    }

   private:
    pid_t m_pid;
    utils::Endianness m_endianness{(std::endian::native == std::endian::little
                                        ? utils::Endianness::LITTLE
                                        : utils::Endianness::BIG)};

    /**
     * @brief Calculate the width of a match based on its flags
     * @param flags Match flags
     * @return Width in bytes
     */
    [[nodiscard]] static auto calculateWidth(
        std::underlying_type_t<MatchFlags> flags) -> std::size_t {
        if ((flags & static_cast<std::underlying_type_t<MatchFlags>>(
                         MatchFlags::B64)) != 0) {
            return 8;
        }
        if ((flags & static_cast<std::underlying_type_t<MatchFlags>>(
                         MatchFlags::B32)) != 0) {
            return 4;
        }
        if ((flags & static_cast<std::underlying_type_t<MatchFlags>>(
                         MatchFlags::B16)) != 0) {
            return 2;
        }
        return 1;
    }

    /**
     * @brief Find the starting address of a segment for a targeted write
     * @param base Base address of the swath
     * @param data Match data array
     * @param index Current index in the array
     * @param flags Match flags
     * @return Starting address and write length
     */
    [[nodiscard]] static auto findSegmentStart(
        std::uint8_t* base, const std::vector<OldValueAndMatchInfo>& data,
        std::size_t index, MatchFlags flags)
        -> std::pair<std::uintptr_t, std::size_t> {
        std::size_t segmentStart = index;
        while (segmentStart > 0) {
            const auto PREVIOUS_FLAGS = data[segmentStart - 1].matchInfo;
            if ((PREVIOUS_FLAGS & flags) != flags) {
                break;
            }
            --segmentStart;
        }
        auto addr = std::bit_cast<std::uintptr_t>(base + segmentStart);
        auto width = calculateWidth(
            static_cast<std::underlying_type_t<MatchFlags>>(flags));
        return {addr, width};
    }

    /**
     * @brief Internal implementation for writing to matches
     * @param scanner Scanner with match results
     * @param value Value to write
     * @param targetIndex Optional specific match index
     * @return Batch write result
     */
    // NOLINTNEXTLINE
    [[nodiscard]] auto writeToMatchesImpl(
        const Scanner& scanner, std::uint64_t value,
        std::optional<std::size_t> targetIndex) const -> BatchWriteResult {
        BatchWriteResult result{.successCount = 0, .failedCount = 0};
        const auto& matches = scanner.getMatches();
        std::size_t currentIndex = 0;
        for (const auto& swath : matches.swaths) {
            auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
            if (base == nullptr) {
                continue;
            }

            for (std::size_t i = 0; i < swath.data.size(); ++i) {
                const auto& cell = swath.data[i];
                if (cell.matchInfo == MatchFlags::EMPTY) {
                    continue;
                }

                // Check if this is the target index (if specified)
                if (targetIndex && currentIndex != *targetIndex) {
                    currentIndex++;
                    continue;
                }

                std::uintptr_t addr = 0;
                std::size_t writeLen = 1;

                if (targetIndex) {
                    // For targeted write, find the segment start
                    std::tie(addr, writeLen) =
                        findSegmentStart(base, swath.data, i, cell.matchInfo);
                } else {
                    // For batch write, write byte by byte
                    addr = std::bit_cast<std::uintptr_t>(base + i);
                }

                // Perform the write
                auto writeRes = write(addr, value, writeLen);

                if (writeRes) {
                    result.successCount++;
                } else {
                    result.failedCount++;
                    result.errors.push_back(
                        std::format("0x{:016x}: {}", addr, writeRes.error()));
                }

                if (targetIndex) {
                    // Only write once for targeted write
                    return result;
                }

                currentIndex++;
            }

            if (targetIndex && result.successCount > 0) {
                break;
            }
        }

        return result;
    }
};

}  // namespace core
