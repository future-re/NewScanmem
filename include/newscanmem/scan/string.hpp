#pragma once

#include <cstdint>
#include <regex>
#include <span>
#include <string_view>
#include <vector>

#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"

[[nodiscard]] auto getCachedRegex(std::string_view pattern) noexcept
    -> const std::regex*;

[[nodiscard]] auto findRegexMatches(
    std::span<const std::uint8_t> memory,
    std::string_view pattern) -> std::vector<ByteMatch>;

[[nodiscard]] auto makeStringScanRoutine(ScanMatchType match_type)
    -> scan::ScanRoutine;
