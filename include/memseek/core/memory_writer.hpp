#pragma once

/**
 * @file memory_writer.hpp
 * @brief Memory writing abstraction using process memory access
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

#include "memseek/core/proc_mem.hpp"
#include "memseek/core/scanner.hpp"
#include "memseek/scan/match_storage.hpp"
#include "memseek/utils/endianness.hpp"
#include "memseek/utils/logging.hpp"
#include "memseek/value/core.hpp"
#include "memseek/value/flags.hpp"

namespace core {

struct WriteResult {
    std::uintptr_t address;
    std::size_t bytesWritten;
    bool success;
};

struct VecWriteResult {
    std::size_t successCount{};
    std::size_t failedCount{};
    std::vector<WriteResult> results;
    std::vector<std::string> errors;
};

class MemoryWriter {
   public:
    explicit MemoryWriter(
        pid_t pid,
        utils::Endianness mode = (std::endian::native == std::endian::little
                                      ? utils::Endianness::LITTLE
                                      : utils::Endianness::BIG));

    auto setEndianness(utils::Endianness mode) -> void;
    [[nodiscard]] auto getEndianness() const noexcept -> utils::Endianness;

    [[nodiscard]] auto writeToMatch(const Scanner& scanner,
                                    const UserValue& value,
                                    std::vector<std::size_t> matchIndices)
        -> std::expected<VecWriteResult, std::string>;

   private:
    [[nodiscard]] auto encodeValueBytes(const UserValue& value) const
        -> std::vector<std::uint8_t>;

    [[nodiscard]] static auto resolveMatchAddresses(
        const scan::MatchesAndOldValuesArray& matches,
        const std::vector<std::size_t>& matchIndices)
        -> std::vector<std::optional<std::uintptr_t>>;

    pid_t m_pid;
    utils::Endianness m_endianness{(std::endian::native == std::endian::little
                                        ? utils::Endianness::LITTLE
                                        : utils::Endianness::BIG)};
};

}  // namespace core
