#pragma once

#include <cstddef>
#include <optional>
#include <regex>
#include <string_view>

#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"

[[nodiscard]] auto getCachedRegex(std::string_view pattern) noexcept
    -> const std::regex*;
[[nodiscard]] auto findRegexPattern(
    const Value* memory, std::size_t memory_length,
    std::string_view pattern) -> std::optional<ByteMatch>;
[[nodiscard]] auto makeStringScanRoutine(ScanMatchType match_type)
    -> scan::ScanRoutine;
