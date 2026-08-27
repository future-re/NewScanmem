#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/utils/read_helpers.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

// This module implements the core numeric matching logic and canonical
// ScanRoutine factories for numeric scan operations.

template <typename T>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline auto numericMatchCore(
    ScanMatchType matchType, T memv, const Value* oldValue,
    const UserValue* userValue, MatchFlags* saveFlags,
    bool reverseEndianness = false) noexcept -> unsigned int {
    const bool NEEDS_USER = matchNeedsUserValue(matchType);

    if (NEEDS_USER && userValue == nullptr) {
        return 0;
    }

    if (NEEDS_USER && userValue != nullptr) {
        const auto REQUIRED = flagForType<T>();
        if ((userValue->flag() & REQUIRED) == MatchFlags::EMPTY) {
            return 0;
        }
    }

    std::optional<T> oldOpt;
    if (matchUsesOldValue(matchType)) {
        oldOpt = oldValueAs<T>(oldValue, reverseEndianness);
        if (!oldOpt) {
            return 0;
        }
    }

    auto markMatched = [&]() -> unsigned int {
        setFlagsIfNotNull(saveFlags, flagForType<T>());
        return sizeof(T);
    };

    auto isEqual = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>) {
            return almostEqual<T>(firstValue, secondValue);
        }
        return firstValue == secondValue;
    };

    auto isNotEqual = [&](T firstValue, T secondValue) {
        return !isEqual(firstValue, secondValue);
    };

    auto isGreaterThan = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>) {
            return firstValue > secondValue &&
                   !isEqual(firstValue, secondValue);
        }
        return firstValue > secondValue;
    };

    auto isLessThan = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>) {
            return firstValue < secondValue &&
                   !isEqual(firstValue, secondValue);
        }
        return firstValue < secondValue;
    };

    std::optional<T> userLowOpt;
    if (NEEDS_USER) {
        userLowOpt = userValueAs<T>(*userValue);
        if (!userLowOpt) {
            return 0;
        }
    }

    const T USERVALUEMAIN = userLowOpt.value_or(T{});

    switch (matchType) {
        case ScanMatchType::MATCH_ANY:
            return markMatched();
        case ScanMatchType::MATCH_EQUAL_TO:
            return isEqual(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCH_NOT_EQUAL_TO:
            return isNotEqual(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCH_GREATER_THAN:
            return isGreaterThan(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCH_LESS_THAN:
            return isLessThan(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCH_UPDATE:
        case ScanMatchType::MATCH_NOT_CHANGED:
            return isEqual(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCH_CHANGED:
            return isNotEqual(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCH_INCREASED:
            return isGreaterThan(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCH_DECREASED:
            return isLessThan(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCH_INCREASED_BY: {
            const T DELTA = memv - *oldOpt;
            if constexpr (std::is_floating_point_v<T>) {
                return almostEqual<T>(DELTA, USERVALUEMAIN) ? markMatched() : 0;
            }
            return (DELTA == USERVALUEMAIN) ? markMatched() : 0;
        }
        case ScanMatchType::MATCH_DECREASED_BY: {
            const T DELTA = *oldOpt - memv;
            if constexpr (std::is_floating_point_v<T>) {
                return almostEqual<T>(DELTA, USERVALUEMAIN) ? markMatched() : 0;
            }
            return (DELTA == USERVALUEMAIN) ? markMatched() : 0;
        }
        case ScanMatchType::MATCH_RANGE: {
            auto highOpt = userValueHighAs<T>(*userValue);
            if (!highOpt || !userLowOpt) {
                return 0;
            }
            const T HIGHVALUE = *highOpt;
            auto [lowBound, highBound] = std::minmax(*userLowOpt, HIGHVALUE);
            if constexpr (std::is_floating_point_v<T>) {
                const T ABS_TOLERANCE = absTol<T>();
                const bool IN_RANGE = (memv >= lowBound - ABS_TOLERANCE &&
                                       memv <= highBound + ABS_TOLERANCE);
                return IN_RANGE ? markMatched() : 0;
            }
            const bool IN_RANGE = (memv >= lowBound && memv <= highBound);
            return IN_RANGE ? markMatched() : 0;
        }
        default:
            return 0;
    }
}

namespace detail {

template <typename T>
inline auto runNumericMatch(ScanMatchType matchType,
                            const scan::ScanContext& ctx,
                            MatchFlags* saveFlags) noexcept -> unsigned int {
    auto memOpt = readTyped<T>(ctx.memory, ctx.reverseEndianness);
    if (!memOpt) {
        return 0;
    }
    return numericMatchCore<T>(matchType, *memOpt,
                               ctx.oldValue ? &*ctx.oldValue : nullptr,
                               ctx.userValue ? &*ctx.userValue : nullptr,
                               saveFlags, ctx.reverseEndianness);
}

template <typename... Ts>
inline auto tryNumericSequence(ScanMatchType matchType,
                               const scan::ScanContext& ctx,
                               MatchFlags* saveFlags) noexcept -> unsigned int {
    unsigned int result = 0;
    ((result != 0 ? 0
                  : result = runNumericMatch<Ts>(matchType, ctx, saveFlags)),
     ...);
    return result;
}

}  // namespace detail

template <typename T>
inline auto makeNumericScanRoutine(
    ScanMatchType matchType, bool reverseEndianness) -> scan::ScanRoutine {
    return [matchType, reverseEndianness](const scan::ScanContext& baseCtx) {
        scan::ScanContext ctx = baseCtx;
        ctx.reverseEndianness = reverseEndianness;
        MatchFlags flags = MatchFlags::EMPTY;
        const auto matched = detail::runNumericMatch<T>(matchType, ctx, &flags);
        if (matched == 0U) {
            return scan::ScanResult::noMatch();
        }
        return scan::ScanResult::match(matched, flags);
    };
}

auto makeAnyIntegerScanRoutine(ScanMatchType matchType,
                               bool reverseEndianness) -> scan::ScanRoutine;

auto makeAnyFloatScanRoutine(ScanMatchType matchType,
                             bool reverseEndianness) -> scan::ScanRoutine;

auto makeAnyNumberScanRoutine(ScanMatchType matchType,
                              bool reverseEndianness) -> scan::ScanRoutine;
