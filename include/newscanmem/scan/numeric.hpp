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

template <typename T>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline auto numericMatchCore(
    ScanMatchType matchType, T memv, const Value* oldValue,
    const UserValue* userValue, MatchFlags* saveFlags,
    bool reverseEndianness = false) noexcept -> unsigned int {
    const bool needsUser = matchNeedsUserValue(matchType);

    if (needsUser && userValue == nullptr) return 0;

    if (needsUser && userValue != nullptr) {
        const auto required = flagForType<T>();
        if ((userValue->flag() & required) == MatchFlags::EMPTY) return 0;
    }

    std::optional<T> oldOpt;
    if (matchUsesOldValue(matchType)) {
        oldOpt = oldValueAs<T>(oldValue, reverseEndianness);
        if (!oldOpt) return 0;
    }

    auto markMatched = [&]() -> unsigned int {
        setFlagsIfNotNull(saveFlags, flagForType<T>());
        return sizeof(T);
    };

    auto isEqual = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>)
            return almostEqual<T>(firstValue, secondValue);
        return firstValue == secondValue;
    };

    auto isNotEqual = [&](T firstValue, T secondValue) {
        return !isEqual(firstValue, secondValue);
    };

    auto isGreaterThan = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>)
            return firstValue > secondValue && !isEqual(firstValue, secondValue);
        return firstValue > secondValue;
    };

    auto isLessThan = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>)
            return firstValue < secondValue && !isEqual(firstValue, secondValue);
        return firstValue < secondValue;
    };

    std::optional<T> userLowOpt;
    if (needsUser) {
        userLowOpt = userValueAs<T>(*userValue);
        if (!userLowOpt) return 0;
    }

    const T userValueMain = userLowOpt.value_or(T{});

    switch (matchType) {
        case ScanMatchType::MATCH_ANY:
            return markMatched();
        case ScanMatchType::MATCH_EQUAL_TO:
            return isEqual(memv, userValueMain) ? markMatched() : 0;
        case ScanMatchType::MATCH_NOT_EQUAL_TO:
            return isNotEqual(memv, userValueMain) ? markMatched() : 0;
        case ScanMatchType::MATCH_GREATER_THAN:
            return isGreaterThan(memv, userValueMain) ? markMatched() : 0;
        case ScanMatchType::MATCH_LESS_THAN:
            return isLessThan(memv, userValueMain) ? markMatched() : 0;
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
            const T delta = memv - *oldOpt;
            if constexpr (std::is_floating_point_v<T>)
                return almostEqual<T>(delta, userValueMain) ? markMatched() : 0;
            return delta == userValueMain ? markMatched() : 0;
        }
        case ScanMatchType::MATCH_DECREASED_BY: {
            const T delta = *oldOpt - memv;
            if constexpr (std::is_floating_point_v<T>)
                return almostEqual<T>(delta, userValueMain) ? markMatched() : 0;
            return delta == userValueMain ? markMatched() : 0;
        }
        case ScanMatchType::MATCH_RANGE: {
            auto highOpt = userValueHighAs<T>(*userValue);
            if (!highOpt || !userLowOpt) return 0;
            const T highValue = *highOpt;
            auto [lowBound, highBound] = std::minmax(*userLowOpt, highValue);
            if constexpr (std::is_floating_point_v<T>) {
                const T absTolerance = absTol<T>();
                const bool inRange =
                    memv >= lowBound - absTolerance &&
                    memv <= highBound + absTolerance;
                return inRange ? markMatched() : 0;
            }
            const bool inRange = memv >= lowBound && memv <= highBound;
            return inRange ? markMatched() : 0;
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
    if (!memOpt) return 0;
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
        if (matched == 0U) return scan::ScanResult::noMatch();
        return scan::ScanResult::match(matched, flags);
    };
}

auto makeAnyIntegerScanRoutine(ScanMatchType matchType,
                               bool reverseEndianness) -> scan::ScanRoutine;

auto makeAnyFloatScanRoutine(ScanMatchType matchType,
                             bool reverseEndianness) -> scan::ScanRoutine;

auto makeAnyNumberScanRoutine(ScanMatchType matchType,
                              bool reverseEndianness) -> scan::ScanRoutine;
