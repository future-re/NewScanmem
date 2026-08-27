#include "newscanmem/scan/string.hpp"

#include <algorithm>
#include <bit>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "newscanmem/scan/bytes.hpp"
#include "newscanmem/value/core.hpp"
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

auto findRegexPattern(const Value* memory, const std::size_t length,
                      const std::string_view pattern)
    -> std::optional<ByteMatch> {
    if (memory == nullptr) return std::nullopt;
    const auto count = std::min(memory->bytes.size(), length);
    const auto* begin = std::bit_cast<const char*>(memory->bytes.data());
    const auto* end = begin + static_cast<std::ptrdiff_t>(count);
    const auto* regex = getCachedRegex(pattern);
    if (regex == nullptr) return std::nullopt;
    std::cmatch match;
    if (!std::regex_search(begin, end, match, *regex)) return std::nullopt;
    return ByteMatch{.offset = static_cast<std::size_t>(match.position()),
                     .length = static_cast<std::size_t>(match.length())};
}

auto makeStringScanRoutine(const ScanMatchType matchType) -> scan::ScanRoutine {
    return [matchType](const scan::ScanContext& context) {
        if (matchType == ScanMatchType::MATCH_ANY)
            return scan::ScanResult::match(context.memory.size(), MatchFlags::B8);
        if (!context.userValue ||
            context.userValue->primary.flag() != MatchFlags::STRING)
            return scan::ScanResult::noMatch();

        const auto& value = context.userValue->primary;
        const std::string_view pattern(
            std::bit_cast<const char*>(value.bytes.data()), value.bytes.size());
        if (pattern.empty()) return scan::ScanResult::noMatch();

        MatchFlags flags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCH_REGEX) {
            const auto* regex = getCachedRegex(pattern);
            if (regex == nullptr) return scan::ScanResult::noMatch();
            const auto* begin = std::bit_cast<const char*>(context.memory.data());
            const auto* end = begin +
                              static_cast<std::ptrdiff_t>(context.memory.size());
            std::cmatch match;
            const auto matched = std::regex_search(
                begin, end, match, *regex,
                std::regex_constants::match_continuous);
            return matched
                       ? scan::ScanResult::match(
                             static_cast<std::size_t>(match.length()),
                             MatchFlags::B8)
                       : scan::ScanResult::noMatch();
        }

        const auto patternBytes = std::span<const std::uint8_t>{
            value.bytes.data(), value.bytes.size()};
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
