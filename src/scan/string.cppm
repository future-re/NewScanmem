module;
#include <algorithm>
#include <bit>
#include <boost/regex.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module scan.string;

import scan.types;
import scan.routine;
import value.flags;
import scan.bytes;
import value.core;

// This module implements string and regex-related routines and a
// thread-local regex cache.
// Exports: makeStringScanRoutine, getCachedRegex, findRegexPattern

export inline auto makeStringScanRoutine(ScanMatchType matchType)
    -> scan::ScanRoutine;

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

export inline auto findRegexPattern(const Value* memoryPtr, size_t memLength,
                                    const std::string& pattern)
    -> std::optional<ByteMatch> {
    if (memoryPtr == nullptr) {
        return std::nullopt;
    }
    const auto& hayAll = memoryPtr->bytes;
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

[[nodiscard]] inline auto handleMATCHANY(size_t memLength,
                                         MatchFlags* saveFlags)
    -> unsigned int {
    setFlagsIfNotNull(saveFlags, MatchFlags::B8);
    return static_cast<unsigned int>(memLength);
}

[[nodiscard]] inline auto runRegexMatch(const Value* memoryPtr,
                                        size_t memLength,
                                        const std::string& pattern,
                                        MatchFlags* saveFlags) -> unsigned int {
    if (memoryPtr == nullptr) {
        return 0;
    }
    const auto& hayAll = memoryPtr->bytes;
    size_t limitSize = std::min(hayAll.size(), memLength);
    std::string hay(hayAll.begin(),
                    hayAll.begin() + static_cast<std::ptrdiff_t>(limitSize));
    if (const auto* rxVal = getCachedRegex(pattern)) {
        boost::smatch matchResult;
        if (boost::regex_search(hay, matchResult, *rxVal)) {
            setFlagsIfNotNull(saveFlags, MatchFlags::B8);
            return static_cast<unsigned int>(matchResult.length());
        }
    }
    return 0;
}

}  // namespace

export inline auto makeStringScanRoutine(ScanMatchType matchType)
    -> scan::ScanRoutine {
    return [matchType](const scan::ScanContext& ctx) -> scan::ScanResult {
        if (matchType == ScanMatchType::MATCH_ANY) {
            return scan::ScanResult::match(ctx.memory.size(), MatchFlags::B8);
        }
        if (!ctx.userValue) {
            return scan::ScanResult::noMatch();
        }
        const auto& patternValue = ctx.userValue->primary;
        if (patternValue.flag() != MatchFlags::STRING) {
            return scan::ScanResult::noMatch();
        }
        std::string_view PATTERN(
            reinterpret_cast<const char*>(patternValue.bytes.data()),
            patternValue.bytes.size());
        if (PATTERN.empty()) {
            return scan::ScanResult::noMatch();
        }
        Value memoryValue{ctx.memory.data(), ctx.memory.size()};
        MatchFlags flags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCH_REGEX) {
            const auto matched =
                runRegexMatch(&memoryValue, ctx.memory.size(),
                              std::string(PATTERN), &flags);
            if (matched == 0U) {
                return scan::ScanResult::noMatch();
            }
            return scan::ScanResult::match(matched, flags);
        }
        const auto* const MASK_PTR =
            (patternValue.mask &&
             patternValue.mask->size() == PATTERN.size())
                ? &*patternValue.mask
                : nullptr;
        const auto* bytePtr = std::bit_cast<const uint8_t*>(PATTERN.data());
        if (MASK_PTR) {
            const auto matched =
                compareBytesMasked(&memoryValue, ctx.memory.size(), bytePtr,
                                   PATTERN.size(), MASK_PTR->data(),
                                   MASK_PTR->size(), &flags);
            if (matched == 0U) {
                return scan::ScanResult::noMatch();
            }
            return scan::ScanResult::match(matched, flags);
        }
        const auto matched =
            compareBytes(&memoryValue, ctx.memory.size(), bytePtr,
                         PATTERN.size(), &flags);
        if (matched == 0U) {
            return scan::ScanResult::noMatch();
        }
        return scan::ScanResult::match(matched, flags);
    };
}
