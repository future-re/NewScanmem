module;
#include <algorithm>
#include <bit>
#include <boost/regex.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

export module scan.string;

import scan.types;
import value.flags;
import scan.bytes;
import value;

// 本模块实现字符串和正则相关例程，以及线程局部的正则缓存。
// 导出：makeStringRoutine, getCachedRegex, findRegexPattern

export inline auto getCachedRegex(const std::string& pattern) noexcept
    -> const boost::regex* {
    thread_local std::string cachedPattern;
    thread_local std::unique_ptr<boost::regex> cachedRegex;
    if (!cachedRegex || cachedPattern != pattern) {
        try {
            cachedRegex =
                std::make_unique<boost::regex>(pattern, boost::regex::perl);
            cachedPattern = pattern;
        } catch (const boost::regex_error&) {
            return nullptr;
        }
    }
    return cachedRegex.get();
}

export inline auto findRegexPattern(const Mem64* memoryPtr, size_t memLength,
                                    const std::string& pattern)
    -> std::optional<ByteMatch> {
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    std::string hay(hayAll.begin(),
                    hayAll.begin() + static_cast<std::ptrdiff_t>(limitSize));
    if (const auto* rxVal = getCachedRegex(pattern)) {
        boost::smatch matchResult;
        if (boost::regex_search(hay, matchResult, *rxVal)) {
            return ByteMatch{
                .offset = static_cast<size_t>(matchResult.position()),
                .length = static_cast<size_t>(matchResult.length())};
        }
    } else {
        return std::nullopt;
    }
    return std::nullopt;
}

namespace {

[[nodiscard]] inline auto handleMatchAny(size_t memLength,
                                         MatchFlags* saveFlags)
    -> unsigned int {
    *saveFlags = MatchFlags::B8;
    return static_cast<unsigned int>(memLength);
}

[[nodiscard]] inline auto runRegexMatch(const Mem64* memoryPtr,
                                        size_t memLength,
                                        const std::string& pattern,
                                        MatchFlags* saveFlags) -> unsigned int {
    auto hayAll = memoryPtr->bytes();
    size_t limitSize = std::min(hayAll.size(), memLength);
    std::string hay(hayAll.begin(),
                    hayAll.begin() + static_cast<std::ptrdiff_t>(limitSize));
    if (const auto* rxVal = getCachedRegex(pattern)) {
        boost::smatch matchResult;
        if (boost::regex_search(hay, matchResult, *rxVal)) {
            *saveFlags = MatchFlags::B8;
            return static_cast<unsigned int>(matchResult.length());
        }
    }
    return 0;
}

}  // namespace

export inline auto makeStringRoutine(ScanMatchType matchType) -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCHANY) {
            return handleMatchAny(memLength, saveFlags);
        }
        if (!userValue) {
            return 0;
        }
        if (matchType == ScanMatchType::MATCHREGEX) {
            return runRegexMatch(memoryPtr, memLength, userValue->stringValue,
                                 saveFlags);
        }
        const std::string_view NEEDLE{userValue->stringValue};
        if (NEEDLE.empty()) {
            return 0;
        }
        const auto* const MASK_PTR =
            (userValue->byteMask &&
             userValue->byteMask->size() == NEEDLE.size())
                ? &*userValue->byteMask
                : nullptr;
        const auto* bytePtr = std::bit_cast<const uint8_t*>(NEEDLE.data());
        auto needleSpan = std::span<const uint8_t>(bytePtr, NEEDLE.size());
        if (MASK_PTR) {
            auto maskSpan =
                std::span<const uint8_t>(MASK_PTR->data(), MASK_PTR->size());
            return compareBytesMasked(memoryPtr, memLength, needleSpan,
                                      maskSpan, saveFlags);
        }
        return compareBytes(memoryPtr, memLength, needleSpan, saveFlags);
    };
}
