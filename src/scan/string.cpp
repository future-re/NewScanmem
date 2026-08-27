#include "newscanmem/scan/string.hpp"

#include <algorithm>
#include <bit>
#include <memory>
#include <string_view>

#include "newscanmem/scan/bytes.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

auto getCachedRegex(const std::string& pattern) noexcept -> const std::regex* {
    thread_local std::string cached_pattern;
    thread_local std::unique_ptr<std::regex> cached_regex;
    if (!cached_regex || cached_pattern != pattern) try {
            cached_regex =
                std::make_unique<std::regex>(pattern, std::regex::ECMAScript);
            cached_pattern = pattern;
        } catch (const std::regex_error&) {
            return nullptr;
        }
    return cached_regex.get();
}
auto findRegexPattern(const Value* memory, const std::size_t length,
                      const std::string& pattern) -> std::optional<ByteMatch> {
    if (memory == nullptr) return std::nullopt;
    const auto count = std::min(memory->bytes.size(), length);
    std::string haystack(
        memory->bytes.begin(),
        memory->bytes.begin() + static_cast<std::ptrdiff_t>(count));
    const auto regex = getCachedRegex(pattern);
    if (regex == nullptr) return std::nullopt;
    std::smatch match;
    return std::regex_search(haystack, match, *regex)
               ? std::optional<ByteMatch>{{.offset = static_cast<std::size_t>(
                                               match.position()),
                                           .length = static_cast<std::size_t>(
                                               match.length())}}
               : std::nullopt;
}
auto makeStringScanRoutine(const ScanMatchType match_type)
    -> scan::ScanRoutine {
    return [match_type](const scan::ScanContext& context) {
        if (match_type == ScanMatchType::MATCH_ANY)
            return scan::ScanResult::match(context.memory.size(),
                                           MatchFlags::B8);
        if (!context.userValue ||
            context.userValue->primary.flag() != MatchFlags::STRING)
            return scan::ScanResult::noMatch();
        const auto& value = context.userValue->primary;
        const std::string_view pattern(
            std::bit_cast<const char*>(value.bytes.data()), value.bytes.size());
        if (pattern.empty()) return scan::ScanResult::noMatch();
        Value memory{context.memory.data(), context.memory.size()};
        MatchFlags flags = MatchFlags::EMPTY;
        if (match_type == ScanMatchType::MATCH_REGEX) {
            const auto match = findRegexPattern(&memory, context.memory.size(),
                                                std::string(pattern));
            return match && match->offset == 0
                       ? scan::ScanResult::match(match->length, MatchFlags::B8)
                       : scan::ScanResult::noMatch();
        }
        const auto* mask = value.mask && value.mask->size() == pattern.size()
                               ? &*value.mask
                               : nullptr;
        const auto bytes = std::bit_cast<const std::uint8_t*>(pattern.data());
        const auto matched =
            mask ? compareBytesMasked(&memory, context.memory.size(), bytes,
                                      pattern.size(), mask->data(),
                                      mask->size(), &flags)
                 : compareBytes(&memory, context.memory.size(), bytes,
                                pattern.size(), &flags);
        return matched ? scan::ScanResult::match(matched, flags)
                       : scan::ScanResult::noMatch();
    };
}
