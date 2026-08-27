#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"

[[nodiscard]] auto compareBytes(const Value* memory, std::size_t memory_length, const std::uint8_t* pattern,
                                std::size_t pattern_length, MatchFlags* flags) -> unsigned int;
[[nodiscard]] auto compareBytes(const Value* memory, std::size_t memory_length, const std::vector<std::uint8_t>& pattern,
                                MatchFlags* flags) -> unsigned int;
[[nodiscard]] auto compareBytesMasked(const Value* memory, std::size_t memory_length, const std::uint8_t* pattern,
    std::size_t pattern_length, const std::uint8_t* mask, std::size_t mask_length, MatchFlags* flags) -> unsigned int;
[[nodiscard]] auto compareBytesMasked(const Value* memory, std::size_t memory_length, const std::vector<std::uint8_t>& pattern,
    const std::vector<std::uint8_t>& mask, MatchFlags* flags) -> unsigned int;
[[nodiscard]] auto findBytePattern(const Value* memory, std::size_t memory_length, const std::uint8_t* pattern,
                                   std::size_t pattern_length) -> std::optional<ByteMatch>;
[[nodiscard]] auto findBytePattern(const Value* memory, std::size_t memory_length, const std::vector<std::uint8_t>& pattern) -> std::optional<ByteMatch>;
[[nodiscard]] auto findBytePatternMasked(const Value* memory, std::size_t memory_length, const std::uint8_t* pattern,
    std::size_t pattern_length, const std::uint8_t* mask, std::size_t mask_length) -> std::optional<ByteMatch>;
[[nodiscard]] auto findBytePatternMasked(const Value* memory, std::size_t memory_length, const std::vector<std::uint8_t>& pattern,
    const std::vector<std::uint8_t>& mask) -> std::optional<ByteMatch>;
[[nodiscard]] auto makeBytearrayScanRoutine(ScanMatchType match_type) -> scan::ScanRoutine;
