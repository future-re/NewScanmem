/**
 * @file memory_writer.cppm
 * @brief Memory writing abstraction using process_vm_writev
 */

module;

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

export module core.memory_writer;

import value.core;

import core.proc_mem;
import core.scanner;
import scan.match_storage;
import utils.endianness;
import value.flags;
import utils.logging;

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
    [[nodiscard]] auto writeToMatch(const Scanner& scanner,
                                    const UserValue& value,
                                    std::vector<size_t> matchIndex)
        -> std::expected<VecWriteResult, std::string> {
        if (m_pid <= 0) {
            return std::unexpected("invalid pid");
        }
        if (matchIndex.empty()) {
            return std::unexpected("no match indices provided");
        }

        const auto bytesToWrite = encodeValueBytes(value);
        if (bytesToWrite.empty()) {
            return std::unexpected("empty write value");
        }

        const auto& matches = scanner.getMatches();
        VecWriteResult summary{
            .successCount = 0,
            .failedCount = 0,
            .results = {},
            .errors = {},
        };

        summary.results.reserve(matchIndex.size());

        for (const auto index : matchIndex) {
            auto addressExp = resolveMatchAddress(matches, index);
            if (!addressExp) {
                summary.failedCount++;
                summary.errors.push_back(addressExp.error());
                continue;
            }

            const auto address = *addressExp;
            auto writeExp =
                core::writeBytes(m_pid, std::bit_cast<void*>(address), bytesToWrite);
            if (!writeExp || *writeExp != bytesToWrite.size()) {
                summary.failedCount++;
                summary.errors.push_back(
                    !writeExp
                        ? std::format("match #{} write failed: {}", index,
                                      writeExp.error())
                        : std::format(
                              "match #{} partial write: expected {} bytes, wrote {}",
                              index, bytesToWrite.size(), *writeExp));
                summary.results.push_back(
                    {.address = address,
                     .bytesWritten = writeExp ? *writeExp : 0,
                     .success = false});
                continue;
            }

            summary.successCount++;
            summary.results.push_back(
                {.address = address,
                 .bytesWritten = *writeExp,
                 .success = true});
        }

        if (summary.successCount == 0) {
            return std::unexpected(summary.errors.empty()
                                       ? "no values written"
                                       : summary.errors.front());
        }

        return summary;
    }

   private:
    [[nodiscard]] auto encodeValueBytes(const UserValue& value) const
        -> std::vector<std::uint8_t> {
        auto bytes = value.primary.bytes;
        if (bytes.size() <= 1) {
            return bytes;
        }
        const auto flags = value.primary.flag();
        const bool isTextual = flags == MatchFlags::STRING ||
                               flags == MatchFlags::BYTE_ARRAY;
        if (isTextual || m_endianness == utils::getHost()) {
            return bytes;
        }
        std::reverse(bytes.begin(), bytes.end());
        return bytes;
    }

    [[nodiscard]] static auto resolveMatchAddress(
        const scan::MatchesAndOldValuesArray& matches, std::size_t matchIndex)
        -> std::expected<std::uintptr_t, std::string> {
        std::size_t currentIndex = 0;
        for (const auto& swath : matches.swaths) {
            if (swath.firstByteInChild == nullptr) {
                continue;
            }
            auto* base =
                static_cast<const std::uint8_t*>(swath.firstByteInChild);
            for (std::size_t i = 0; i < swath.data.size(); ++i) {
                if (swath.data[i].matchInfo == MatchFlags::EMPTY) {
                    continue;
                }
                if (currentIndex == matchIndex) {
                    return std::bit_cast<std::uintptr_t>(base + i);
                }
                currentIndex++;
            }
        }
        return std::unexpected(
            std::format("match index {} out of range", matchIndex));
    }

    pid_t m_pid;
    utils::Endianness m_endianness{(std::endian::native == std::endian::little
                                        ? utils::Endianness::LITTLE
                                        : utils::Endianness::BIG)};
};

}  // namespace core
