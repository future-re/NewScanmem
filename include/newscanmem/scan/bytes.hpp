#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"

[[nodiscard]] auto compareBytes(std::span<const std::uint8_t> memory,
                                std::span<const std::uint8_t> pattern,
                                MatchFlags* flags) -> unsigned int;
[[nodiscard]] auto compareBytesMasked(
    std::span<const std::uint8_t> memory,
    std::span<const std::uint8_t> pattern,
    std::span<const std::uint8_t> mask,
    MatchFlags* flags) -> unsigned int;

[[nodiscard]] auto findBytePattern(
    std::span<const std::uint8_t> memory,
    std::span<const std::uint8_t> pattern) -> std::optional<ByteMatch>;
[[nodiscard]] auto findBytePatternMasked(
    std::span<const std::uint8_t> memory,
    std::span<const std::uint8_t> pattern,
    std::span<const std::uint8_t> mask) -> std::optional<ByteMatch>;

[[nodiscard]] auto makeBytearrayScanRoutine(ScanMatchType match_type)
    -> scan::ScanRoutine;
