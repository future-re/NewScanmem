module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

export module scan.bytes;

import scan.types;
import value.flags;

import value;

// This module provides helpers for comparing and searching byte arrays/strings
// (complex regex not included).
// Exports: compareBytes, compareBytesMasked, findBytePattern,
//          findBytePatternMasked, makeBytearrayRoutine
//
// Conventions (very important):
// - compareBytes/compareBytesMasked only compare whether the prefix starting
//   at the current position matches. That is, they do not search within the
//   current buffer — the scan engine advances the offset and calls them.
// - If you need to find the first occurrence inside a buffer, use the
//   findBytePattern* functions (search semantics).

export inline auto compareBytes(const Mem64* memoryPtr, size_t memLength,
                                std::span<const std::uint8_t> needle,
                                MatchFlags* saveFlags) -> unsigned int {
    if (needle.empty()) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    if (limitSize < needle.size()) {
        return 0;
    }
    auto hay = std::span<const std::uint8_t>(hayAll.data(), limitSize);
    if (std::equal(needle.begin(), needle.end(), hay.begin())) {
        *saveFlags = MatchFlags::B8;
        return static_cast<unsigned int>(needle.size());
    }
    return 0;
}

export inline auto compareBytesMasked(const Mem64* memoryPtr, size_t memLength,
                                      std::span<const std::uint8_t> needle,
                                      std::span<const std::uint8_t> mask,
                                      MatchFlags* saveFlags) -> unsigned int {
    if (needle.empty() || mask.size() != needle.size()) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    if (limitSize < needle.size()) {
        return 0;
    }
    auto hay = std::span<const std::uint8_t>(hayAll.data(), limitSize);
    const size_t NEEDLE_SIZE = needle.size();
    for (size_t j = 0; j < NEEDLE_SIZE; ++j) {
        if (((hay[j] ^ needle[j]) & mask[j]) != 0) {
            return 0;
        }
    }
    *saveFlags = MatchFlags::B8;
    return static_cast<unsigned int>(NEEDLE_SIZE);
}

export inline auto findBytePattern(const Mem64* memoryPtr, size_t memLength,
                                   std::span<const std::uint8_t> needle)
    -> std::optional<ByteMatch> {
    if (needle.empty()) {
        return std::nullopt;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t LIMIT = std::min(hayAll.size(), memLength);
    if (LIMIT < needle.size()) {
        return std::nullopt;
    }
    auto hay = std::span<const std::uint8_t>(hayAll.data(), LIMIT);
    auto iter = std::ranges::search(hay, needle).begin();
    if (iter == hay.end()) {
        return std::nullopt;
    }
    auto off = static_cast<size_t>(std::distance(hay.begin(), iter));
    return ByteMatch{.offset = off, .length = needle.size()};
}

export inline auto findBytePatternMasked(const Mem64* memoryPtr,
                                         size_t memLength,
                                         std::span<const std::uint8_t> needle,
                                         std::span<const std::uint8_t> mask)
    -> std::optional<ByteMatch> {
    if (needle.empty() || mask.size() != needle.size()) {
        return std::nullopt;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t LIMIT = std::min(hayAll.size(), memLength);
    if (LIMIT < needle.size()) {
        return std::nullopt;
    }
    auto hay = std::span<const std::uint8_t>(hayAll.data(), LIMIT);
    size_t needleSize = needle.size();
    for (size_t hayIndex = 0; hayIndex + needleSize <= hay.size(); ++hayIndex) {
        bool matched = true;
        for (size_t needleIndex = 0; needleIndex < needleSize; ++needleIndex) {
            if (((hay[hayIndex + needleIndex] ^ needle[needleIndex]) &
                 mask[needleIndex]) != 0) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return ByteMatch{.offset = hayIndex, .length = needleSize};
        }
    }
    return std::nullopt;
}

export inline auto makeBytearrayRoutine(ScanMatchType matchType)
    -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCHANY) {
            *saveFlags = MatchFlags::B8;
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue || !userValue->bytearrayValue) {
            return 0;
        }
        const auto& byteArrayRef = *userValue->bytearrayValue;
        auto needle = std::span<const std::uint8_t>(byteArrayRef.data(),
                                                    byteArrayRef.size());
        if (userValue->byteMask &&
            userValue->byteMask->size() == byteArrayRef.size()) {
            auto mask = std::span<const std::uint8_t>(
                userValue->byteMask->data(), userValue->byteMask->size());
            return compareBytesMasked(memoryPtr, memLength, needle, mask,
                                      saveFlags);
        }
        return compareBytes(memoryPtr, memLength, needle, saveFlags);
    };
}
