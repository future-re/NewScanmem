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

// Byte compare & search helpers used by the scanning engine.
// Public utilities:
// - compareBytes / compareBytesMasked: check whether the buffer prefix at the
//   current offset matches the supplied pattern (NOT a search).
// - findBytePattern / findBytePatternMasked: search for the first occurrence.
// - makeBytearrayRoutine: build a scan routine for byte array matches.

export inline auto compareBytes(const Mem64* memoryPtr, size_t memLength,
                                const std::uint8_t* patternData,
                                size_t patternSize, MatchFlags* saveFlags)
    -> unsigned int {
    if (patternData == nullptr || patternSize == 0) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    if (limitSize < patternSize) {
        return 0;
    }
    if (std::equal(patternData, patternData + patternSize, hayAll.begin())) {
        setFlagsIfNotNull(saveFlags, MatchFlags::B8);
        return static_cast<unsigned int>(patternSize);
    }
    return 0;
}

// Convenience overload for vector
export inline auto compareBytes(const Mem64* memoryPtr, size_t memLength,
                                const std::vector<std::uint8_t>& pattern,
                                MatchFlags* saveFlags) -> unsigned int {
    return compareBytes(memoryPtr, memLength, pattern.data(), pattern.size(),
                        saveFlags);
}

export inline auto compareBytesMasked(const Mem64* memoryPtr, size_t memLength,
                                      const std::uint8_t* patternData,
                                      size_t patternSize,
                                      const std::uint8_t* maskData,
                                      size_t maskSize, MatchFlags* saveFlags)
    -> unsigned int {
    if (patternData == nullptr || maskData == nullptr || patternSize == 0 ||
        maskSize != patternSize) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    if (limitSize < patternSize) {
        return 0;
    }
    const size_t PATTERN_SIZE = patternSize;
    for (size_t j = 0; j < PATTERN_SIZE; ++j) {
        if (((hayAll[j] ^ patternData[j]) & maskData[j]) != 0) {
            return 0;
        }
    }
    setFlagsIfNotNull(saveFlags, (MatchFlags::B8 | MatchFlags::BYTE_ARRAY));
    return static_cast<unsigned int>(PATTERN_SIZE);
}

// Convenience overload for vectors
export inline auto compareBytesMasked(const Mem64* memoryPtr, size_t memLength,
                                      const std::vector<std::uint8_t>& pattern,
                                      const std::vector<std::uint8_t>& mask,
                                      MatchFlags* saveFlags) -> unsigned int {
    return compareBytesMasked(memoryPtr, memLength, pattern.data(),
                              pattern.size(), mask.data(), mask.size(),
                              saveFlags);
}

export inline auto findBytePattern(const Mem64* memoryPtr, size_t memLength,
                                   const std::uint8_t* patternData,
                                   size_t patternSize)
    -> std::optional<ByteMatch> {
    if (patternData == nullptr || patternSize == 0) {
        return std::nullopt;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t LIMIT = std::min(hayAll.size(), memLength);
    if (LIMIT < patternSize) {
        return std::nullopt;
    }
    auto hayView = hayAll | std::views::take(LIMIT);
    auto patternView = std::views::counted(
        patternData, static_cast<std::ptrdiff_t>(patternSize));
    auto searchResult = std::ranges::search(hayView, patternView);
    if (searchResult.empty()) {
        return std::nullopt;
    }
    auto off = static_cast<size_t>(
        std::ranges::distance(hayView.begin(), searchResult.begin()));
    return ByteMatch{.offset = off, .length = patternSize};
}

// Convenience overload for vector
export inline auto findBytePattern(const Mem64* memoryPtr, size_t memLength,
                                   const std::vector<std::uint8_t>& pattern)
    -> std::optional<ByteMatch> {
    return findBytePattern(memoryPtr, memLength, pattern.data(),
                           pattern.size());
}

export inline auto findBytePatternMasked(
    const Mem64* memoryPtr, size_t memLength, const std::uint8_t* patternData,
    size_t patternSize, const std::uint8_t* maskData, size_t maskSize)
    -> std::optional<ByteMatch> {
    if (patternData == nullptr || maskData == nullptr || patternSize == 0 ||
        maskSize != patternSize) {
        return std::nullopt;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t LIMIT = std::min(hayAll.size(), memLength);
    if (LIMIT < patternSize) {
        return std::nullopt;
    }
    for (size_t hayIndex = 0; hayIndex + patternSize <= LIMIT; ++hayIndex) {
        bool matched = true;
        for (size_t patternIndex = 0; patternIndex < patternSize;
             ++patternIndex) {
            if (((hayAll[hayIndex + patternIndex] ^ patternData[patternIndex]) &
                 maskData[patternIndex]) != 0) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return ByteMatch{.offset = hayIndex, .length = patternSize};
        }
    }
    return std::nullopt;
}

// Convenience overload for vectors
export inline auto findBytePatternMasked(
    const Mem64* memoryPtr, size_t memLength,
    const std::vector<std::uint8_t>& pattern,
    const std::vector<std::uint8_t>& mask) -> std::optional<ByteMatch> {
    return findBytePatternMasked(memoryPtr, memLength, pattern.data(),
                                 pattern.size(), mask.data(), mask.size());
}

export inline auto makeBytearrayRoutine(ScanMatchType matchType)
    -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        setFlagsIfNotNull(saveFlags, MatchFlags::EMPTY);
        if (matchType == ScanMatchType::MATCH_ANY) {
            setFlagsIfNotNull(saveFlags, MatchFlags::B8);
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue || !userValue->bytearrayValue) {
            return 0;
        }
        const auto& byteArrayRef = *userValue->bytearrayValue;
        if (userValue->byteMask &&
            userValue->byteMask->size() == byteArrayRef.size()) {
            unsigned matchedLen = compareBytesMasked(
                memoryPtr, memLength, byteArrayRef.data(), byteArrayRef.size(),
                userValue->byteMask->data(), userValue->byteMask->size(),
                saveFlags);
            if (matchedLen > 0) {
                orFlagsIfNotNull(saveFlags, MatchFlags::BYTE_ARRAY);
            }
            return matchedLen;
        }
        unsigned matchedLen =
            compareBytes(memoryPtr, memLength, byteArrayRef.data(),
                         byteArrayRef.size(), saveFlags);
        if (matchedLen > 0) {
            orFlagsIfNotNull(saveFlags, MatchFlags::BYTE_ARRAY);
        }
        return matchedLen;
    };
}
