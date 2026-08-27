#include "newscanmem/scan/bytes.hpp"

#include <algorithm>
#include <span>

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

auto findBytePattern(const std::span<const std::uint8_t> memory,
                     const std::span<const std::uint8_t> pattern)
    -> std::optional<ByteMatch> {
    if (pattern.empty() || memory.size() < pattern.size()) return std::nullopt;

    const auto found = std::search(memory.begin(), memory.end(), pattern.begin(),
                                   pattern.end());
    if (found == memory.end()) return std::nullopt;

    return ByteMatch{.offset = static_cast<std::size_t>(found - memory.begin()),
                     .length = pattern.size()};
}

auto findBytePatternMasked(const std::span<const std::uint8_t> memory,
                           const std::span<const std::uint8_t> pattern,
                           const std::span<const std::uint8_t> mask)
    -> std::optional<ByteMatch> {
    if (pattern.empty() || mask.size() != pattern.size() ||
        memory.size() < pattern.size())
        return std::nullopt;

    for (std::size_t start = 0; start + pattern.size() <= memory.size();
         ++start) {
        bool matches = true;
        for (std::size_t index = 0; index < pattern.size(); ++index) {
            if (((memory[start + index] ^ pattern[index]) & mask[index]) != 0) {
                matches = false;
                break;
            }
        }
        if (matches) return ByteMatch{.offset = start, .length = pattern.size()};
    }

    return std::nullopt;
}

auto makeBytearrayScanRoutine(const ScanMatchType match) -> scan::ScanRoutine {
    return [match](const scan::ScanContext& context) {
        MatchFlags flags = MatchFlags::EMPTY;
        if (match == ScanMatchType::MATCH_ANY)
            return scan::ScanResult::match(context.memory.size(), MatchFlags::B8);
        if (!context.userValue ||
            context.userValue->flag() != MatchFlags::BYTE_ARRAY)
            return scan::ScanResult::noMatch();

        const auto& pattern = context.userValue->primary;
        if (pattern.bytes.empty()) return scan::ScanResult::noMatch();

        const auto patternView = std::span<const std::uint8_t>{pattern.bytes};
        const auto result =
            pattern.mask && pattern.mask->size() == pattern.bytes.size()
                ? compareBytesMasked(
                      context.memory, patternView,
                      std::span<const std::uint8_t>{*pattern.mask}, &flags)
                : compareBytes(context.memory, patternView, &flags);

        return result ? scan::ScanResult::match(result,
                                                flags | MatchFlags::BYTE_ARRAY)
                      : scan::ScanResult::noMatch();
    };
}
