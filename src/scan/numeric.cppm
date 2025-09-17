module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>

export module scan.numeric;

import scan.types;
import scan.read_helpers;
import value;

// 本模块实现数值类型的匹配核心与工厂函数。
// 导出：numericMatchCore、makeNumericRoutine、makeAnyIntegerRoutine、makeAnyFloatRoutine、makeAnyNumberRoutine

export template <typename T>
inline auto numericMatchCore(ScanMatchType matchType, T memv,
                             const Value* oldValue, const UserValue* userValue,
                             MatchFlags* saveFlags) noexcept -> unsigned int {
    const bool NEEDS_USER = (matchType == ScanMatchType::MATCHEQUALTO ||
                             matchType == ScanMatchType::MATCHNOTEQUALTO ||
                             matchType == ScanMatchType::MATCHGREATERTHAN ||
                             matchType == ScanMatchType::MATCHLESSTHAN ||
                             matchType == ScanMatchType::MATCHINCREASEDBY ||
                             matchType == ScanMatchType::MATCHDECREASEDBY ||
                             matchType == ScanMatchType::MATCHRANGE);
    if (NEEDS_USER && userValue == nullptr) {
        return 0;
    }
    if (NEEDS_USER && userValue != nullptr) {
        const auto REQUIRED = flagForType<T>();
        if ((userValue->flags & REQUIRED) == MatchFlags::EMPTY) {
            return 0;
        }
    }

    auto oldOpt = oldValueAs<T>(oldValue);
    auto getUser = [&]() -> T { return userValueAs<T>(*userValue); };

    bool isMatched = false;
    auto isEqual = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>) {
            return almostEqual<T>(firstValue, secondValue);
        } else {
            return firstValue == secondValue;
        }
    };
    auto isNotEqual = [&](T firstValue, T secondValue) {
        return !isEqual(firstValue, secondValue);
    };
    auto isGreaterThan = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>) {
            return firstValue > secondValue &&
                   !isEqual(firstValue, secondValue);
        } else {
            return firstValue > secondValue;
        }
    };
    auto isLessThan = [&](T firstValue, T secondValue) {
        if constexpr (std::is_floating_point_v<T>) {
            return firstValue < secondValue &&
                   !isEqual(firstValue, secondValue);
        } else {
            return firstValue < secondValue;
        }
    };

    switch (matchType) {
        case ScanMatchType::MATCHANY:
            isMatched = true;
            break;
        case ScanMatchType::MATCHEQUALTO:
            isMatched = isEqual(memv, getUser());
            break;
        case ScanMatchType::MATCHNOTEQUALTO:
            isMatched = isNotEqual(memv, getUser());
            break;
        case ScanMatchType::MATCHGREATERTHAN:
            isMatched = isGreaterThan(memv, getUser());
            break;
        case ScanMatchType::MATCHLESSTHAN:
            isMatched = isLessThan(memv, getUser());
            break;
        case ScanMatchType::MATCHUPDATE:
        case ScanMatchType::MATCHNOTCHANGED:
            if (oldOpt) {
                isMatched = isEqual(memv, *oldOpt);
            }
            break;
        case ScanMatchType::MATCHCHANGED:
            if (oldOpt) {
                isMatched = isNotEqual(memv, *oldOpt);
            }
            break;
        case ScanMatchType::MATCHINCREASED:
            if (oldOpt) {
                isMatched = isGreaterThan(memv, *oldOpt);
            }
            break;
        case ScanMatchType::MATCHDECREASED:
            if (oldOpt) {
                isMatched = isLessThan(memv, *oldOpt);
            }
            break;
        case ScanMatchType::MATCHINCREASEDBY:
            if (oldOpt) {
                if constexpr (std::is_floating_point_v<T>) {
                    isMatched = almostEqual<T>(memv - *oldOpt, getUser());
                } else {
                    isMatched = (memv - *oldOpt == getUser());
                }
            }
            break;
        case ScanMatchType::MATCHDECREASEDBY:
            if (oldOpt) {
                if constexpr (std::is_floating_point_v<T>) {
                    isMatched = almostEqual<T>(*oldOpt - memv, getUser());
                } else {
                    isMatched = (*oldOpt - memv == getUser());
                }
            }
            break;
        case ScanMatchType::MATCHRANGE: {
            if (userValue) {
                T low = userValueAs<T>(*userValue);
                T high = userValueHighAs<T>(*userValue);
                if constexpr (std::is_floating_point_v<T>) {
                    const T ABS_TOLERANCE = absTol<T>();
                    auto [lowBound, highBound] = std::minmax(low, high);
                    isMatched = (memv >= lowBound - ABS_TOLERANCE &&
                                 memv <= highBound + ABS_TOLERANCE);
                } else {
                    auto [lowBound, highBound] = std::minmax(low, high);
                    isMatched = (memv >= lowBound && memv <= highBound);
                }
            }
            break;
        }
        default:
            isMatched = false;
            break;
    }
    if (!isMatched) {
        return 0;
    }
    *saveFlags = flagForType<T>();
    return sizeof(T);
}

export template <typename T>
inline auto makeNumericRoutine(ScanMatchType matchType, bool reverseEndianness)
    -> scanRoutine {
    return
        [matchType, reverseEndianness](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            auto memOpt = readTyped<T>(memoryPtr, memLength, reverseEndianness);
            if (!memOpt) {
                return 0;
            }
            *saveFlags = MatchFlags::EMPTY;
            return numericMatchCore<T>(matchType, *memOpt, oldValue, userValue,
                                       saveFlags);
        };
}

export inline auto makeAnyIntegerRoutine(ScanMatchType matchType,
                                         bool reverseEndianness)
    -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        auto tryOne = [&](auto tag) -> unsigned {
            using T = decltype(tag);
            auto memOpt = readTyped<T>(memoryPtr, memLength, reverseEndianness);
            if (!memOpt) {
                return 0;
            }
            return numericMatchCore<T>(matchType, *memOpt, oldValue, userValue,
                                       saveFlags);
        };
        if (auto resultValue = tryOne(uint64_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(int64_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(uint32_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(int32_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(uint16_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(int16_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(uint8_t{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(int8_t{})) {
            return resultValue;
        }
        return 0;
    };
}

export inline auto makeAnyFloatRoutine(ScanMatchType matchType,
                                       bool reverseEndianness) -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        auto tryOne = [&](auto tag) -> unsigned {
            using T = decltype(tag);
            auto memOpt = readTyped<T>(memoryPtr, memLength, reverseEndianness);
            if (!memOpt) {
                return 0;
            }
            return numericMatchCore<T>(matchType, *memOpt, oldValue, userValue,
                                       saveFlags);
        };
        if (auto resultValue = tryOne(double{})) {
            return resultValue;
        }
        if (auto resultValue = tryOne(float{})) {
            return resultValue;
        }
        return 0;
    };
}

export inline auto makeAnyNumberRoutine(ScanMatchType matchType,
                                        bool reverseEndianness) -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        if (auto resultValue =
                makeAnyFloatRoutine(matchType, reverseEndianness)(
                    memoryPtr, memLength, oldValue, userValue, saveFlags)) {
            return resultValue;
        }
        return makeAnyIntegerRoutine(matchType, reverseEndianness)(
            memoryPtr, memLength, oldValue, userValue, saveFlags);
    };
}
