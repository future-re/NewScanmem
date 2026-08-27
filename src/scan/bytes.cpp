#include "newscanmem/scan/bytes.hpp"

#include <algorithm>

#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

auto compareBytes(const Value* memory, const std::size_t memory_length,
                  const std::uint8_t* pattern, const std::size_t pattern_length,
                  MatchFlags* flags) -> unsigned int {
    if (memory == nullptr || pattern == nullptr || pattern_length == 0 ||
        std::min(memory->bytes.size(), memory_length) < pattern_length)
        return 0;
    if (!std::equal(pattern, pattern + pattern_length, memory->bytes.begin()))
        return 0;
    setFlagsIfNotNull(flags, MatchFlags::B8);
    return static_cast<unsigned>(pattern_length);
}
auto compareBytes(const Value* memory, const std::size_t length,
                  const std::vector<std::uint8_t>& pattern,
                  MatchFlags* flags) -> unsigned int {
    return compareBytes(memory, length, pattern.data(), pattern.size(), flags);
}
auto compareBytesMasked(const Value* memory, const std::size_t memory_length,
                        const std::uint8_t* pattern,
                        const std::size_t pattern_length,
                        const std::uint8_t* mask, const std::size_t mask_length,
                        MatchFlags* flags) -> unsigned int {
    if (memory == nullptr || pattern == nullptr || mask == nullptr ||
        pattern_length == 0 || mask_length != pattern_length ||
        std::min(memory->bytes.size(), memory_length) < pattern_length)
        return 0;
    for (std::size_t index = 0; index < pattern_length; ++index)
        if (((memory->bytes[index] ^ pattern[index]) & mask[index]) != 0)
            return 0;
    setFlagsIfNotNull(flags, MatchFlags::B8 | MatchFlags::BYTE_ARRAY);
    return static_cast<unsigned>(pattern_length);
}
auto compareBytesMasked(const Value* memory, const std::size_t length,
                        const std::vector<std::uint8_t>& pattern,
                        const std::vector<std::uint8_t>& mask,
                        MatchFlags* flags) -> unsigned int {
    return compareBytesMasked(memory, length, pattern.data(), pattern.size(),
                              mask.data(), mask.size(), flags);
}
auto findBytePattern(const Value* memory, const std::size_t memory_length,
                     const std::uint8_t* pattern,
                     const std::size_t pattern_length)
    -> std::optional<ByteMatch> {
    if (memory == nullptr || pattern == nullptr || pattern_length == 0)
        return std::nullopt;
    const auto limit = std::min(memory->bytes.size(), memory_length);
    if (limit < pattern_length) return std::nullopt;
    const auto found =
        std::search(memory->bytes.begin(),
                    memory->bytes.begin() + static_cast<std::ptrdiff_t>(limit),
                    pattern, pattern + pattern_length);
    return found == memory->bytes.begin() + static_cast<std::ptrdiff_t>(limit)
               ? std::nullopt
               : std::optional<ByteMatch>{{.offset = static_cast<std::size_t>(
                                               found - memory->bytes.begin()),
                                           .length = pattern_length}};
}
auto findBytePattern(const Value* memory, const std::size_t length,
                     const std::vector<std::uint8_t>& pattern)
    -> std::optional<ByteMatch> {
    return findBytePattern(memory, length, pattern.data(), pattern.size());
}
auto findBytePatternMasked(
    const Value* memory, const std::size_t memory_length,
    const std::uint8_t* pattern, const std::size_t pattern_length,
    const std::uint8_t* mask,
    const std::size_t mask_length) -> std::optional<ByteMatch> {
    if (memory == nullptr || pattern == nullptr || mask == nullptr ||
        pattern_length == 0 || mask_length != pattern_length)
        return std::nullopt;
    const auto limit = std::min(memory->bytes.size(), memory_length);
    for (std::size_t start = 0; start + pattern_length <= limit; ++start) {
        bool matches = true;
        for (std::size_t index = 0; index < pattern_length; ++index)
            if (((memory->bytes[start + index] ^ pattern[index]) &
                 mask[index]) != 0) {
                matches = false;
                break;
            }
        if (matches)
            return ByteMatch{.offset = start, .length = pattern_length};
    }
    return std::nullopt;
}
auto findBytePatternMasked(const Value* memory, const std::size_t length,
                           const std::vector<std::uint8_t>& pattern,
                           const std::vector<std::uint8_t>& mask)
    -> std::optional<ByteMatch> {
    return findBytePatternMasked(memory, length, pattern.data(), pattern.size(),
                                 mask.data(), mask.size());
}
auto makeBytearrayScanRoutine(const ScanMatchType match) -> scan::ScanRoutine {
    return [match](const scan::ScanContext& context) {
        MatchFlags flags = MatchFlags::EMPTY;
        if (match == ScanMatchType::MATCH_ANY)
            return scan::ScanResult::match(context.memory.size(),
                                           MatchFlags::B8);
        if (!context.userValue ||
            context.userValue->flag() != MatchFlags::BYTE_ARRAY)
            return scan::ScanResult::noMatch();
        const auto& pattern = context.userValue->primary;
        if (pattern.bytes.empty()) return scan::ScanResult::noMatch();
        Value memory{context.memory.data(), context.memory.size()};
        const auto result =
            pattern.mask && pattern.mask->size() == pattern.bytes.size()
                ? compareBytesMasked(&memory, context.memory.size(),
                                     pattern.bytes.data(), pattern.bytes.size(),
                                     pattern.mask->data(), pattern.mask->size(),
                                     &flags)
                : compareBytes(&memory, context.memory.size(),
                               pattern.bytes.data(), pattern.bytes.size(),
                               &flags);
        return result ? scan::ScanResult::match(result,
                                                flags | MatchFlags::BYTE_ARRAY)
                      : scan::ScanResult::noMatch();
    };
}
