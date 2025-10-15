module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

export module scan.numeric;

import scan.types;
import scan.read_helpers;
import value.flags;
import value;

// 本模块实现数值类型的匹配核心与工厂函数。
// 导出：numericMatchCore、makeNumericRoutine、makeAnyIntegerRoutine、makeAnyFloatRoutine、makeAnyNumberRoutine

export template <typename T>
inline auto numericMatchCore(ScanMatchType matchType, T memv,
                             const Value* oldValue, const UserValue* userValue,
                             MatchFlags* saveFlags) noexcept -> unsigned int {
    const bool NEEDS_USER = matchNeedsUserValue(matchType);
    if (NEEDS_USER && userValue == nullptr) {
        return 0;
    }
    if (NEEDS_USER && userValue != nullptr) {
        const auto REQUIRED = flagForType<T>();
        if ((userValue->flags & REQUIRED) == MatchFlags::EMPTY) {
            return 0;
        }
    }

    std::optional<T> oldOpt;
    if (matchUsesOldValue(matchType)) {
        oldOpt = oldValueAs<T>(oldValue);
        if (!oldOpt) {
            return 0;
        }
    }

    auto markMatched = [&]() -> unsigned int {
        *saveFlags = flagForType<T>();
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

    const T USERVALUEMAIN = NEEDS_USER ? userValueAs<T>(*userValue) : T{};

    switch (matchType) {
        case ScanMatchType::MATCHANY:
            return markMatched();
        case ScanMatchType::MATCHEQUALTO:
            return isEqual(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCHNOTEQUALTO:
            return isNotEqual(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCHGREATERTHAN:
            return isGreaterThan(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCHLESSTHAN:
            return isLessThan(memv, USERVALUEMAIN) ? markMatched() : 0;
        case ScanMatchType::MATCHUPDATE:
        case ScanMatchType::MATCHNOTCHANGED:
            return isEqual(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCHCHANGED:
            return isNotEqual(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCHINCREASED:
            return isGreaterThan(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCHDECREASED:
            return isLessThan(memv, *oldOpt) ? markMatched() : 0;
        case ScanMatchType::MATCHINCREASEDBY: {
            const T DELTA = memv - *oldOpt;
            if constexpr (std::is_floating_point_v<T>) {
                return almostEqual<T>(DELTA, USERVALUEMAIN) ? markMatched() : 0;
            }
            return (DELTA == USERVALUEMAIN) ? markMatched() : 0;
        }
        case ScanMatchType::MATCHDECREASEDBY: {
            const T DELTA = *oldOpt - memv;
            if constexpr (std::is_floating_point_v<T>) {
                return almostEqual<T>(DELTA, USERVALUEMAIN) ? markMatched() : 0;
            }
            return (DELTA == USERVALUEMAIN) ? markMatched() : 0;
        }
        case ScanMatchType::MATCHRANGE: {
            const T HIGHVALUE = userValueHighAs<T>(*userValue);
            auto [lowBound, highBound] = std::minmax(USERVALUEMAIN, HIGHVALUE);
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
inline auto runNumericMatch(ScanMatchType matchType, bool reverseEndianness,
                            const Mem64* memoryPtr, size_t memLength,
                            const Value* oldValue, const UserValue* userValue,
                            MatchFlags* saveFlags) noexcept -> unsigned int {
    auto memOpt = readTyped<T>(memoryPtr, memLength, reverseEndianness);
    if (!memOpt) {
        return 0;
    }
    return numericMatchCore<T>(matchType, *memOpt, oldValue, userValue,
                               saveFlags);
}

template <typename... Ts>
inline auto tryNumericSequence(ScanMatchType matchType, bool reverseEndianness,
                               const Mem64* memoryPtr, size_t memLength,
                               const Value* oldValue,
                               const UserValue* userValue,
                               MatchFlags* saveFlags) noexcept -> unsigned int {
    unsigned int result = 0;
    ((result != 0 ? 0
                  : result = runNumericMatch<Ts>(matchType, reverseEndianness,
                                                 memoryPtr, memLength, oldValue,
                                                 userValue, saveFlags)),
     ...);
    return result;
}

}  // namespace detail

export template <typename T>
inline auto makeNumericRoutine(ScanMatchType matchType, bool reverseEndianness)
    -> scanRoutine {
    return
        [matchType, reverseEndianness](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            *saveFlags = MatchFlags::EMPTY;
            return detail::runNumericMatch<T>(matchType, reverseEndianness,
                                              memoryPtr, memLength, oldValue,
                                              userValue, saveFlags);
        };
}

export inline auto makeAnyIntegerRoutine(ScanMatchType matchType,
                                         bool reverseEndianness)
    -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        return detail::tryNumericSequence<uint64_t, int64_t, uint32_t, int32_t,
                                          uint16_t, int16_t, uint8_t, int8_t>(
            matchType, reverseEndianness, memoryPtr, memLength, oldValue,
            userValue, saveFlags);
    };
}

export inline auto makeAnyFloatRoutine(ScanMatchType matchType,
                                       bool reverseEndianness) -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        return detail::tryNumericSequence<double, float>(
            matchType, reverseEndianness, memoryPtr, memLength, oldValue,
            userValue, saveFlags);
    };
}

export inline auto makeAnyNumberRoutine(ScanMatchType matchType,
                                        bool reverseEndianness) -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        if (auto resultValue = detail::tryNumericSequence<double, float>(
                matchType, reverseEndianness, memoryPtr, memLength, oldValue,
                userValue, saveFlags)) {
            return resultValue;
        }
        return detail::tryNumericSequence<uint64_t, int64_t, uint32_t, int32_t,
                                          uint16_t, int16_t, uint8_t, int8_t>(
            matchType, reverseEndianness, memoryPtr, memLength, oldValue,
            userValue, saveFlags);
    };
}
