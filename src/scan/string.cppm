module;

#include <boost/regex.hpp>
#include <memory>
#include <optional>
#include <string>

export module scan.string;

import scan.types;
import value;

// 本模块实现字符串和正则相关例程，以及线程局部的正则缓存。
// 导出：makeStringRoutine, getCachedRegex, findRegexPattern

export inline const boost::regex* getCachedRegex(
    const std::string& pattern) noexcept {
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
    if (const auto* rx = getCachedRegex(pattern)) {
        boost::smatch matchResult;
        if (boost::regex_search(hay, matchResult, *rx)) {
            return ByteMatch{
                .offset = static_cast<size_t>(matchResult.position()),
                .length = static_cast<size_t>(matchResult.length())};
        }
    } else {
        return std::nullopt;
    }
    return std::nullopt;
}

export inline auto makeStringRoutine(ScanMatchType matchType) -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCHANY) {
            *saveFlags = MatchFlags::B8;
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue) {
            return 0;
        }
        if (matchType == ScanMatchType::MATCHREGEX) {
            auto hayAll = memoryPtr->bytes();
            size_t limitSize = std::min(hayAll.size(), memLength);
            std::string hay(
                hayAll.begin(),
                hayAll.begin() + static_cast<std::ptrdiff_t>(limitSize));
            if (const auto* rx = getCachedRegex(userValue->stringValue)) {
                boost::smatch matchResult;
                if (boost::regex_search(hay, matchResult, *rx)) {
                    *saveFlags = MatchFlags::B8;
                    return static_cast<unsigned int>(matchResult.length());
                }
            } else {
                return 0;
            }
            return 0;
        }
        const auto& stringRef = userValue->stringValue;
        auto needle = std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(stringRef.data()),
            stringRef.size());
        if (userValue->byteMask &&
            userValue->byteMask->size() == stringRef.size()) {
            auto mask = std::span<const uint8_t>(userValue->byteMask->data(),
                                                 userValue->byteMask->size());
            return compareBytesMasked(memoryPtr, memLength, needle, mask,
                                      saveFlags);
        }
        return compareBytes(memoryPtr, memLength, needle, saveFlags);
    };
}
