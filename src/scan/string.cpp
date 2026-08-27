#include "newscanmem/scan/string.hpp"

#include <bit>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/scan/bytes.hpp"
#include "newscanmem/value/flags.hpp"

auto getCachedRegex(const std::string_view pattern) noexcept -> const std::regex* {
    thread_local std::string cachedPattern;
    thread_local std::unique_ptr<std::regex> cachedRegex;

    if (!cachedRegex || cachedPattern != pattern) {
        try {
            cachedPattern.assign(pattern);
            cachedRegex =
                std::make_unique<std::regex>(cachedPattern, std::regex::ECMAScript);
        } catch (const std::regex_error&) {
            cachedRegex.reset();
            return nullptr;
        }
    }

    return cachedRegex.get();
}

auto findRegexMatches(const std::span<const std::uint8_t> memory,
                      const std::string_view pattern)
    -> std::vector<ByteMatch> {
    std::vector<ByteMatch> matches;
    if (memory.empty() || pattern.empty()) return matches;

    const auto* regex = getCachedRegex(pattern);
    if (regex == nullptr) return matches;

    const auto* begin = std::bit_cast<const char*>(memory.data());
    const auto* end = begin + static_cast<std::ptrdiff_t>(memory.size());
    const char* cursor = begin;

    while (cursor <= end) {
        std::cmatch match;
        if (!std::regex_search(cursor, end, match, *regex)) break;

        const auto* matchBegin = match[0].first;
        const auto* matchEnd = match[0].second;
        const auto offset = static_cast<std::size_t>(matchBegin - begin);
        const auto length = static_cast<std::size_t>(matchEnd - matchBegin);

        // Zero-length matches are not useful for memory scanning and would
        // otherwise make progress handling ambiguous.
        if (length != 0) {
            matches.push_back({.offset = offset, .length = length});
        }

        // Advance from the match start rather than the match end so overlapping
        // matches remain discoverable (e.g. "aba" in "ababa").
        if (matchBegin < end) {
            cursor = matchBegin + 1;
        } else {
            break;
        }
    }

    return matches;
}

auto makeStringScanRoutine(const ScanMatchType matchType) -> scan::ScanRoutine {
    return [matchType](const scan::ScanContext& context) {
        if (matchType == ScanMatchType::MATCH_ANY)
            return scan::ScanResult::match(context.memory.size(), MatchFlags::B8);

        if (matchType == ScanMatchType::MATCH_REGEX)
            return scan::ScanResult::noMatch();

        if (!context.userValue ||
            context.userValue->primary.flag() != MatchFlags::STRING)
            return scan::ScanResult::noMatch();

        const auto& value = context.userValue->primary;
        const auto patternBytes = std::span<const std::uint8_t>{value.bytes};
        if (patternBytes.empty()) return scan::ScanResult::noMatch();

        MatchFlags flags = MatchFlags::EMPTY;
        const auto matched =
            value.mask && value.mask->size() == patternBytes.size()
                ? compareBytesMasked(
                      context.memory, patternBytes,
                      std::span<const std::uint8_t>{*value.mask}, &flags)
                : compareBytes(context.memory, patternBytes, &flags);

        return matched ? scan::ScanResult::match(matched, flags)
                       : scan::ScanResult::noMatch();
    };
}
