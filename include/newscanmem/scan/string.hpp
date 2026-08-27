#pragma once

#include <cstddef>
#include <optional>
#include <regex>
#include <string>

#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"

[[nodiscard]] auto getCachedRegex(const std::string& pattern) noexcept -> const std::regex*;
[[nodiscard]] auto findRegexPattern(const Value* memory, std::size_t memory_length, const std::string& pattern) -> std::optional<ByteMatch>;
[[nodiscard]] auto makeStringScanRoutine(ScanMatchType match_type) -> scan::ScanRoutine;
