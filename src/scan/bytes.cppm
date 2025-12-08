module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
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
                                const std::uint8_t* needleData,
                                size_t needleSize, MatchFlags* saveFlags)
    -> unsigned int {
    if (needleData == nullptr || needleSize == 0) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    if (limitSize < needleSize) {
        return 0;
    }
    if (std::equal(needleData, needleData + needleSize, hayAll.begin())) {
        *saveFlags = MatchFlags::B8;
        return static_cast<unsigned int>(needleSize);
    }
    return 0;
}

// Convenience overload for vector
export inline auto compareBytes(const Mem64* memoryPtr, size_t memLength,
                                const std::vector<std::uint8_t>& needle,
                                MatchFlags* saveFlags) -> unsigned int {
    return compareBytes(memoryPtr, memLength, needle.data(), needle.size(),
                        saveFlags);
}

export inline auto compareBytesMasked(const Mem64* memoryPtr, size_t memLength,
                                      const std::uint8_t* needleData,
                                      size_t needleSize,
                                      const std::uint8_t* maskData,
                                      size_t maskSize, MatchFlags* saveFlags)
    -> unsigned int {
    if (needleData == nullptr || maskData == nullptr || needleSize == 0 ||
        maskSize != needleSize) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    if (limitSize < needleSize) {
        return 0;
    }
    const size_t NEEDLE_SIZE = needleSize;
    for (size_t j = 0; j < NEEDLE_SIZE; ++j) {
        if (((hayAll[j] ^ needleData[j]) & maskData[j]) != 0) {
            return 0;
        }
    }
    *saveFlags = MatchFlags::B8;
    return static_cast<unsigned int>(NEEDLE_SIZE);
}

// Convenience overload for vectors
export inline auto compareBytesMasked(const Mem64* memoryPtr, size_t memLength,
                                      const std::vector<std::uint8_t>& needle,
                                      const std::vector<std::uint8_t>& mask,
                                      MatchFlags* saveFlags) -> unsigned int {
    return compareBytesMasked(memoryPtr, memLength, needle.data(),
                              needle.size(), mask.data(), mask.size(),
                              saveFlags);
}

export inline auto findBytePattern(const Mem64* memoryPtr, size_t memLength,
                                   const std::uint8_t* needleData,
                                   size_t needleSize)
    -> std::optional<ByteMatch> {
    if (needleData == nullptr || needleSize == 0) {
        return std::nullopt;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t LIMIT = std::min(hayAll.size(), memLength);
    if (LIMIT < needleSize) {
        return std::nullopt;
    }
    auto hayView = hayAll | std::views::take(LIMIT);
    auto needleView = std::views::counted(
        needleData, static_cast<std::ptrdiff_t>(needleSize));
    auto searchResult = std::ranges::search(hayView, needleView);
    if (searchResult.empty()) {
        return std::nullopt;
    }
    auto off = static_cast<size_t>(
        std::ranges::distance(hayView.begin(), searchResult.begin()));
    return ByteMatch{.offset = off, .length = needleSize};
}

// Convenience overload for vector
export inline auto findBytePattern(const Mem64* memoryPtr, size_t memLength,
                                   const std::vector<std::uint8_t>& needle)
    -> std::optional<ByteMatch> {
    return findBytePattern(memoryPtr, memLength, needle.data(), needle.size());
}

export inline auto findBytePatternMasked(
    const Mem64* memoryPtr, size_t memLength, const std::uint8_t* needleData,
    size_t needleSize, const std::uint8_t* maskData, size_t maskSize)
    -> std::optional<ByteMatch> {
    if (needleData == nullptr || maskData == nullptr || needleSize == 0 ||
        maskSize != needleSize) {
        return std::nullopt;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t LIMIT = std::min(hayAll.size(), memLength);
    if (LIMIT < needleSize) {
        return std::nullopt;
    }
    for (size_t hayIndex = 0; hayIndex + needleSize <= LIMIT; ++hayIndex) {
        bool matched = true;
        for (size_t needleIndex = 0; needleIndex < needleSize; ++needleIndex) {
            if (((hayAll[hayIndex + needleIndex] ^ needleData[needleIndex]) &
                 maskData[needleIndex]) != 0) {
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

// Convenience overload for vectors
export inline auto findBytePatternMasked(
    const Mem64* memoryPtr, size_t memLength,
    const std::vector<std::uint8_t>& needle,
    const std::vector<std::uint8_t>& mask) -> std::optional<ByteMatch> {
    return findBytePatternMasked(memoryPtr, memLength, needle.data(),
                                 needle.size(), mask.data(), mask.size());
}

export inline auto makeBytearrayRoutine(ScanMatchType matchType)
    -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCH_ANY) {
            *saveFlags = MatchFlags::B8;
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue || !userValue->bytearrayValue) {
            return 0;
        }
        const auto& byteArrayRef = *userValue->bytearrayValue;
        if (userValue->byteMask &&
            userValue->byteMask->size() == byteArrayRef.size()) {
            return compareBytesMasked(memoryPtr, memLength, byteArrayRef.data(),
                                      byteArrayRef.size(),
                                      userValue->byteMask->data(),
                                      userValue->byteMask->size(), saveFlags);
        }
        return compareBytes(memoryPtr, memLength, byteArrayRef.data(),
                            byteArrayRef.size(), saveFlags);
    };
}
