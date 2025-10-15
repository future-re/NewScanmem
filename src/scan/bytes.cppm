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

// 本模块实现字节数组/字符串的比较与查找辅助（不包含复杂正则）
// 导出：compareBytes, compareBytesMasked, findBytePattern,
//       findBytePatternMasked, makeBytearrayRoutine
//
// 约定（非常重要）：
// - compareBytes/compareBytesMasked 仅比较“当前位置起始处”的前缀是否匹配。
//   即：它们不会在当前缓冲内做搜索，而是由“扫描引擎”逐偏移推进并调用。
//   若需要在一段缓冲区内查找首次出现的位置，请使用 findBytePattern*
//   （搜索语义）。

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
