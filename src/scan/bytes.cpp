#include "newscanmem/scan/bytes.hpp"

#include <algorithm>
#include <span>

#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

auto compareBytes(const std::span<const std::uint8_t> memory,
                  const std::span<const std::uint8_t> pattern,
                  MatchFlags* flags) -> unsigned int {
    if (pattern.empty() || memory.size() < pattern.size()) return 0;
    if (!std::equal(pattern.begin(), pattern.end(), memory.begin())) return 0;
    setFlagsIfNotNull(flags, MatchFlags::B8);
    return static_cast<unsigned>(pattern.size());
}

auto compareBytesMasked(const std::span<const std::uint8_t> memory,
                        const std::span<const std::uint8_t> pattern,
                        const std::span<const std::uint8_t> mask,
                        MatchFlags* flags) -> unsigned int {
    if (pattern.empty() || mask.size() != pattern.size() ||
        memory.size() < pattern.size())
        return 0;
    for (std::size_t index = 0; index < pattern.size(); ++index) {
        if (((memory[index] ^ pattern[index]) & mask[index]) != 0) return 0;
    }
    setFlagsIfNotNull(flags, MatchFlags::B8 | MatchFlags::BYTE_ARRAY);
    return static_cast<unsigned>(pattern.size());
}

auto compareBytes(const Value* memory, const std::size_t memoryLength,
                  const std::uint8_t* pattern, const std::size_t patternLength,
                  MatchFlags* flags) -> unsigned int {
    if (memory == nullptr || pattern == nullptr) return 0;
    const auto length = std::min(memory->bytes.size(), memoryLength);
    return compareBytes(std::span<const std::uint8_t>{memory->bytes.data(), length},
                        std::span<const std::uint8_t>{pattern, patternLength},
                        flags);
}

auto compareBytes(const Value* memory, const std::size_t length,
                  const std::vector<std::uint8_t>& pattern,
                  MatchFlags* flags) -> unsigned int {
    return compareBytes(memory, length, pattern.data(), pattern.size(), flags);
}

auto compareBytesMasked(const Value* memory, const std::size_t memoryLength,
                        const std::uint8_t* pattern,
                        const std::size_t patternLength,
                        const std::uint8_t* mask, const std::size_t maskLength,
                        MatchFlags* flags) -> unsigned int {
    if (memory == nullptr || pattern == nullptr || mask == nullptr) return 0;
    const auto length = std::min(memory->bytes.size(), memoryLength);
    return compareBytesMasked(
        std::span<const std::uint8_t>{memory->bytes.data(), length},
        std::span<const std::uint8_t>{pattern, patternLength},
        std::span<const std::uint8_t>{mask, maskLength}, flags);
}

auto compareBytesMasked(const Value* memory, const std::size_t length,
                        const std::vector<std::uint8_t>& pattern,
                        const std::vector<std::uint8_t>& mask,
                        MatchFlags* flags) -> unsigned int {
    return compareBytesMasked(memory, length, pattern.data(), pattern.size(),
                              mask.data(), mask.size(), flags);
}

auto findBytePattern(const Value* memory, const std::size_t memoryLength,
                     const std::uint8_t* pattern,
                     const std::size_t patternLength)
    -> std::optional<ByteMatch> {
    if (memory == nullptr || pattern == nullptr || patternLength == 0)
        return std::nullopt;
    const auto limit = std::min(memory->bytes.size(), memoryLength);
    if (limit < patternLength) return std::nullopt;
    const auto found =
        std::search(memory->bytes.begin(),
                    memory->bytes.begin() + static_cast<std::ptrdiff_t>(limit),
                    pattern, pattern + patternLength);
    return found == memory->bytes.begin() + static_cast<std::ptrdiff_t>(limit)
               ? std::nullopt
               : std::optional<ByteMatch>{{.offset = static_cast<std::size_t>(
                                               found - memory->bytes.begin()),
                                           .length = patternLength}};
}

auto findBytePattern(const Value* memory, const std::size_t length,
                     const std::vector<std::uint8_t>& pattern)
    -> std::optional<ByteMatch> {
    return findBytePattern(memory, length, pattern.data(), pattern.size());
}

auto findBytePatternMasked(
    const Value* memory, const std::size_t memoryLength,
    const std::uint8_t* pattern, const std::size_t patternLength,
    const std::uint8_t* mask,
    const std::size_t maskLength) -> std::optional<ByteMatch> {
    if (memory == nullptr || pattern == nullptr || mask == nullptr ||
        patternLength == 0 || maskLength != patternLength)
        return std::nullopt;
    const auto limit = std::min(memory->bytes.size(), memoryLength);
    for (std::size_t start = 0; start + patternLength <= limit; ++start) {
        bool matches = true;
        for (std::size_t index = 0; index < patternLength; ++index) {
            if (((memory->bytes[start + index] ^ pattern[index]) & mask[index]) != 0) {
                matches = false;
                break;
            }
        }
        if (matches) return ByteMatch{.offset = start, .length = patternLength};
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
            return scan::ScanResult::match(context.memory.size(), MatchFlags::B8);
        if (!context.userValue || context.userValue->flag() != MatchFlags::BYTE_ARRAY)
            return scan::ScanResult::noMatch();
        const auto& pattern = context.userValue->primary;
        if (pattern.bytes.empty()) return scan::ScanResult::noMatch();
        const auto patternView = std::span<const std::uint8_t>{pattern.bytes};
        const auto result = pattern.mask && pattern.mask->size() == pattern.bytes.size()
                                ? compareBytesMasked(
                                      context.memory, patternView,
                                      std::span<const std::uint8_t>{*pattern.mask}, &flags)
                                : compareBytes(context.memory, patternView, &flags);
        return result ? scan::ScanResult::match(result, flags | MatchFlags::BYTE_ARRAY)
                      : scan::ScanResult::noMatch();
    };
}
