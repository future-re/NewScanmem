module;

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <boost/regex.hpp>
#include <string>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

export module scanroutines;

import value;

template <typename T>
struct TypeTraits;

template <>
struct TypeTraits<int8_t> {
    static constexpr auto FLAG = MatchFlags::S8B;
    static constexpr auto LOW = &UserValue::int8Value;
    static constexpr auto HIGH = &UserValue::int8RangeHighValue;
};
template <>
struct TypeTraits<uint8_t> {
    static constexpr auto FLAG = MatchFlags::U8B;
    static constexpr auto LOW = &UserValue::uint8Value;
    static constexpr auto HIGH = &UserValue::uint8RangeHighValue;
};
template <>
struct TypeTraits<int16_t> {
    static constexpr auto FLAG = MatchFlags::S16B;
    static constexpr auto LOW = &UserValue::int16Value;
    static constexpr auto HIGH = &UserValue::int16RangeHighValue;
};
template <>
struct TypeTraits<uint16_t> {
    static constexpr auto FLAG = MatchFlags::U16B;
    static constexpr auto LOW = &UserValue::uint16Value;
    static constexpr auto HIGH = &UserValue::uint16RangeHighValue;
};
template <>
struct TypeTraits<int32_t> {
    static constexpr auto FLAG = MatchFlags::S32B;
    static constexpr auto LOW = &UserValue::int32Value;
    static constexpr auto HIGH = &UserValue::int32RangeHighValue;
};
template <>
struct TypeTraits<uint32_t> {
    static constexpr auto FLAG = MatchFlags::U32B;
    static constexpr auto LOW = &UserValue::uint32Value;
    static constexpr auto HIGH = &UserValue::uint32RangeHighValue;
};
template <>
struct TypeTraits<int64_t> {
    static constexpr auto FLAG = MatchFlags::S64B;
    static constexpr auto LOW = &UserValue::int64Value;
    static constexpr auto HIGH = &UserValue::int64RangeHighValue;
};
template <>
struct TypeTraits<uint64_t> {
    static constexpr auto FLAG = MatchFlags::U64B;
    static constexpr auto LOW = &UserValue::uint64Value;
    static constexpr auto HIGH = &UserValue::uint64RangeHighValue;
};
template <>
struct TypeTraits<float> {
    static constexpr auto FLAG = MatchFlags::F32B;
    static constexpr auto LOW = &UserValue::float32Value;
    static constexpr auto HIGH = &UserValue::float32RangeHighValue;
};
template <>
struct TypeTraits<double> {
    static constexpr auto FLAG = MatchFlags::F64B;
    static constexpr auto LOW = &UserValue::float64Value;
    static constexpr auto HIGH = &UserValue::float64RangeHighValue;
};

/**
 * 扫描数据类型枚举
 *
 * 说明：用于描述用户在界面/调用中选择要匹配的值的类型。
 * ANY* 系列表示不区分具体位宽或整数/浮点的高级分类，
 * INTEGER8..FLOAT64 表示精确的字节宽度用于内存读取。
 * BYTEARRAY/STRING 表示按字节序列比较的情形。
 */
export enum class ScanDataType {
    ANYNUMBER,  /* ANYINTEGER or ANYFLOAT */
    ANYINTEGER, /* INTEGER of whatever width */
    ANYFLOAT,   /* FLOAT of whatever width */
    INTEGER8,   /* 8 位有符号整数 */
    INTEGER16,  /* 16 位有符号整数 */
    INTEGER32,  /* 32 位有符号整数 */
    INTEGER64,  /* 64 位有符号整数 */
    FLOAT32,    /* 32 位单精度浮点数 */
    FLOAT64,    /* 64 位双精度浮点数 */
    BYTEARRAY,  /* 通常表示原始字节序列（raw bytes），没有字符编码含义（比如
                   std::vector<uint8_t>、std::span<std::byte>
                   或者二进制缓冲区）。*/
    STRING      /* 通常表示文本字符串，带字符语义和编码（比如 std::string /
                   std::u8string / std::string_view，通常假定为 UTF-8
                   或某种字符编码）。*/
};

/**
 * 扫描匹配类型枚举
 *
 * 描述需要执行的比较种类：快照（MATCHANY）、与给定值比较（MATCHEQUALTO 等）、
 * 与旧值比较（MATCHUPDATE 等）、或使用给定值与旧值的复合比较（MATCHINCREASEDBY
 * 等）。
 */
export enum class ScanMatchType {
    MATCHANY, /* 快照（snapshot） */
    /* 以下：与给定值比较 */
    MATCHEQUALTO,     /* 等于（与 userValue 比较，==） */
    MATCHNOTEQUALTO,  /* 不等于（与 userValue 比较，!=） */
    MATCHGREATERTHAN, /* 大于（与 userValue 比较，>） */
    MATCHLESSTHAN,    /* 小于（与 userValue 比较，<） */
    MATCHRANGE,       /* 范围匹配（未在当前实现中提供） */
    MATCHREGEX,       /* 字符串正则匹配（STRING 专用） */
    /* 以下：与快照比较 */
    MATCHUPDATE, /* 当前位置 == 先前快照（当前实现与 MATCHNOTCHANGED 等价） */
    MATCHNOTCHANGED,  /* 当前位置 == 先前快照（未变化） */
    MATCHCHANGED,     /* 当前位置 != 先前快照（已变化） */
    MATCHINCREASED,   /* 当前位置 > 先前快照（数值增大） */
    MATCHDECREASED,   /* 当前位置 < 先前快照（数值减小） */
    MATCHINCREASEDBY, /* (current - old) == userValue（需要 userValue） */
    MATCHDECREASEDBY  /* (old - current) == userValue（需要 userValue） */
};

/**
 * scanRoutine: 单个扫描位置的匹配函数签名
 *
 * 返回值：匹配时返回匹配的字节数（>=1），未匹配返回 0。
 * 参数：
 *  - memoryPtr: 指向包含待解释字节的 Mem64 封装（实现可用
 * memoryPtr->get<T>()）。
 *  - memLength: 当前地址可用的字节数（用于长度检查）。
 *  - oldValue: 可选的先前快照值（用于 MATCHUPDATE/MATCHCHANGED 等）。
 *  - userValue: 可选的用户提供值（用于 MATCHEQUALTO 等）。
 *  - saveFlags: 输出参数，匹配成功时写入值类型/宽度标志。
 */
export using scanRoutine = std::function<unsigned int(
    const Mem64* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;

// 按需交换字节序（仅当 reverse==true）。对整数使用 swapBytesIntegral；
// 对浮点通过 bit_cast 到等宽无符号整数再交换字节序。
template <typename T>
constexpr auto swapIfReverse(T value, bool reverse) noexcept -> T {
    if (!reverse) {
        return value;
    }
    if constexpr (std::is_integral_v<T>) {
        return std::byteswap(value);
    } else if constexpr (std::is_same_v<T, float>) {
        auto bits32 = std::bit_cast<uint32_t>(value);
        bits32 = std::byteswap(bits32);
        return std::bit_cast<float>(bits32);
    } else if constexpr (std::is_same_v<T, double>) {
        auto bits64 = std::bit_cast<uint64_t>(value);
        bits64 = std::byteswap(bits64);
        return std::bit_cast<double>(bits64);
    } else {
        return value;
    }
}

// 浮点比较（带容差）
template <typename F>
constexpr auto relTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-5F);
    } else {
        return static_cast<F>(1E-12);
    }
}
template <typename F>
constexpr auto absTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-6F);
    } else {
        return static_cast<F>(1E-12);
    }
}

template <typename F>
[[nodiscard]] inline auto almostEqual(F firstValue, F secondValue) noexcept
    -> bool {
    using std::fabs;
    const F DIFFERENCE_VALUE = fabs(firstValue - secondValue);
    const F SCALE_VALUE =
        std::max(F(1), std::max(fabs(firstValue), fabs(secondValue)));
    return DIFFERENCE_VALUE <= std::max(absTol<F>(), relTol<F>() * SCALE_VALUE);
}

// Helpers: map C++ type -> MatchFlags flag for the size/type
//
// 说明：根据模板类型 T 返回对应的 MatchFlags，用于在匹配时记录匹配的类型/宽度。
template <typename T>
[[nodiscard]] constexpr auto flagForType() noexcept -> MatchFlags {
    return TypeTraits<T>::FLAG;
}

// Extract user value for given type T from UserValue
/**
 * 从 UserValue 中按模板类型提取用户值
 *
 * 注意：调用方应确保 userValue 已经存在且与 matchType 要求一致。
 */
template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& userValue) noexcept
    -> T {
    return userValue.*(TypeTraits<T>::LOW);
}

// 获取上界（用于 MATCHRANGE）
template <typename T>
[[nodiscard]] inline auto userValueHighAs(const UserValue& userValue) noexcept
    -> T {
    return userValue.*(TypeTraits<T>::HIGH);
}

// Try to get old value of type T from Value*
/**
 * 尝试从 Value* 中按模板类型读取先前快照值
 *
 * 返回：存在则返回 std::optional 包裹的值，否者返回 std::nullopt。
 */
template <typename T>
[[nodiscard]] inline auto oldValueAs(const Value* valuePtr) noexcept
    -> std::optional<T> {
    if (!valuePtr) {
        return std::nullopt;
    }
    // 严格校验：flags 必须与期望类型匹配
    const auto required = flagForType<T>();
    if ((valuePtr->flags & required) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    if (valuePtr->view().size() < sizeof(T)) {
        return std::nullopt;
    }
    T out{};
    std::memcpy(&out, valuePtr->view().data(), sizeof(T));
    return out;
}

// 统一的数值匹配核心：依据 matchType 对单个内存值做判定
// 返回匹配成功时的 sizeof(T)，否则 0。
template <typename T>
inline auto numericMatchCore(ScanMatchType matchType, T memv,
                             const Value* oldValue, const UserValue* userValue,
                             MatchFlags* saveFlags) noexcept -> unsigned int {
    // 匹配类型是否需要 userValue
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

    auto oldOpt = oldValueAs<T>(oldValue);
    auto getUser = [&](void) -> T { return userValueAs<T>(*userValue); };

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
        case ScanMatchType::MATCHUPDATE:  // 与 NOTCHANGED 归并
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

// 数值类型扫描例程工厂
template <typename T>
inline auto makeNumericRoutine(ScanMatchType matchType, bool reverseEndianness)
    -> scanRoutine {
    return
        [matchType, reverseEndianness](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            if (memLength < sizeof(T)) {
                return 0;
            }
            T memv;
            try {
                memv = memoryPtr->get<T>();
            } catch (const std::bad_variant_access&) {
                return 0;
            }
            memv = swapIfReverse<T>(memv, reverseEndianness);
            *saveFlags = MatchFlags::EMPTY;
            return numericMatchCore<T>(matchType, memv, oldValue, userValue,
                                       saveFlags);
        };
}

// 统一的字节序列比较（字符串/字节数组皆可经过视图转换后复用）
// 简单滑窗搜索版本：在当前可读范围内查找子序列
inline auto compareBytes(const Mem64* memoryPtr, size_t memLength,
                         std::span<const uint8_t> needle,
                         MatchFlags* saveFlags) -> unsigned int {
    if (needle.empty()) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t limit = std::min(hayAll.size(), memLength);
    if (limit < needle.size()) {
        return 0;
    }
    auto hay = std::span<const uint8_t>(hayAll.data(), limit);
    auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end());
    if (it != hay.end()) {
        *saveFlags = MatchFlags::B8;
        return static_cast<unsigned int>(needle.size());
    }
    return 0;
}

// 带掩码的滑窗比较：((hay ^ needle) & mask) == 0 即视为匹配
inline auto compareBytesMasked(const Mem64* memoryPtr, size_t memLength,
                               std::span<const uint8_t> needle,
                               std::span<const uint8_t> mask,
                               MatchFlags* saveFlags) -> unsigned int {
    if (needle.empty() || mask.size() != needle.size()) {
        return 0;
    }
    auto hayAll = memoryPtr->bytes();
    const size_t limit = std::min(hayAll.size(), memLength);
    if (limit < needle.size()) {
        return 0;
    }
    auto hay = std::span<const uint8_t>(hayAll.data(), limit);
    const size_t n = needle.size();
    for (size_t i = 0; i + n <= hay.size(); ++i) {
        bool ok = true;
        for (size_t j = 0; j < n; ++j) {
            if (((hay[i + j] ^ needle[j]) & mask[j]) != 0) {
                ok = false;
                break;
            }
        }
        if (ok) {
            *saveFlags = MatchFlags::B8;
            return static_cast<unsigned int>(n);
        }
    }
    return 0;
}

// 辅助：导出可获取偏移量的查找结果，便于在 targetmem 中做范围标记
export struct ByteMatch {
    size_t offset;
    size_t length;
};

export inline auto findBytePattern(const Mem64* memoryPtr, size_t memLength,
                                   std::span<const uint8_t> needle)
    -> std::optional<ByteMatch> {
    if (needle.empty()) return std::nullopt;
    auto hayAll = memoryPtr->bytes();
    const size_t limit = std::min(hayAll.size(), memLength);
    if (limit < needle.size()) return std::nullopt;
    auto hay = std::span<const uint8_t>(hayAll.data(), limit);
    auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end());
    if (it == hay.end()) return std::nullopt;
    size_t off = static_cast<size_t>(std::distance(hay.begin(), it));
    return ByteMatch{off, needle.size()};
}

// 带掩码的查找版本
export inline auto findBytePatternMasked(const Mem64* memoryPtr,
                                         size_t memLength,
                                         std::span<const uint8_t> needle,
                                         std::span<const uint8_t> mask)
    -> std::optional<ByteMatch> {
    if (needle.empty() || mask.size() != needle.size()) return std::nullopt;
    auto hayAll = memoryPtr->bytes();
    const size_t limit = std::min(hayAll.size(), memLength);
    if (limit < needle.size()) return std::nullopt;
    auto hay = std::span<const uint8_t>(hayAll.data(), limit);
    const size_t n = needle.size();
    for (size_t i = 0; i + n <= hay.size(); ++i) {
        bool ok = true;
        for (size_t j = 0; j < n; ++j) {
            if (((hay[i + j] ^ needle[j]) & mask[j]) != 0) {
                ok = false;
                break;
            }
        }
        if (ok) return ByteMatch{i, n};
    }
    return std::nullopt;
}

inline auto makeBytearrayRoutine(ScanMatchType matchType) -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCHANY) {
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue || !userValue->bytearrayValue) {
            return 0;
        }
        const auto& v = *userValue->bytearrayValue;
        auto needle = std::span<const uint8_t>(v.data(), v.size());
        if (userValue->byteMask && userValue->byteMask->size() == v.size()) {
            auto mask = std::span<const uint8_t>(userValue->byteMask->data(),
                                                 userValue->byteMask->size());
            return compareBytesMasked(memoryPtr, memLength, needle, mask,
                                      saveFlags);
        }
        return compareBytes(memoryPtr, memLength, needle, saveFlags);
    };
}

inline auto makeStringRoutine(ScanMatchType matchType) -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCHANY) {
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue) {
            return 0;
        }
        if (matchType == ScanMatchType::MATCHREGEX) {
            auto hayAll = memoryPtr->bytes();
            const size_t limit = std::min(hayAll.size(), memLength);
            std::string hay(reinterpret_cast<const char*>(hayAll.data()), limit);
            try {
                const boost::regex re(userValue->stringValue,
                                      boost::regex::perl);
                boost::smatch m;
                if (boost::regex_search(hay, m, re)) {
                    *saveFlags = MatchFlags::B8;
                    return static_cast<unsigned int>(m.length());
                }
            } catch (const boost::regex_error&) {
                return 0;
            }
            return 0;
        } else {
            const auto& s = userValue->stringValue;
            auto needle = std::span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(s.data()), s.size());
            if (userValue->byteMask && userValue->byteMask->size() == s.size()) {
                auto mask = std::span<const uint8_t>(
                    userValue->byteMask->data(), userValue->byteMask->size());
                return compareBytesMasked(memoryPtr, memLength, needle, mask,
                                          saveFlags);
            }
            return compareBytes(memoryPtr, memLength, needle, saveFlags);
        }
    };
}

// 正则匹配获取偏移与长度（STRING 专用）
export inline auto findRegexPattern(const Mem64* memoryPtr, size_t memLength,
                                    const std::string& pattern)
    -> std::optional<ByteMatch> {
    auto hayAll = memoryPtr->bytes();
    const size_t limit = std::min(hayAll.size(), memLength);
    std::string hay(reinterpret_cast<const char*>(hayAll.data()), limit);
    try {
        const boost::regex re(pattern, boost::regex::perl);
        boost::smatch m;
        if (boost::regex_search(hay, m, re)) {
            return ByteMatch{static_cast<size_t>(m.position()),
                             static_cast<size_t>(m.length())};
        }
    } catch (const boost::regex_error&) {
        return std::nullopt;
    }
    return std::nullopt;
}

// 读取当前值并保存为旧值（统一端序与 flags）
template <typename T>
export inline void captureOldValue(const Mem64* memoryPtr,
                                   bool reverseEndianness, Value& out) {
    if (!memoryPtr) {
        Value::zero(out);
        return;
    }
    try {
        T v = memoryPtr->get<T>();
        v = swapIfReverse<T>(v, reverseEndianness);
        out.setScalarTyped<T>(v);
    } catch (...) {
        Value::zero(out);
    }
}

// ANY* 聚合例程：基于字节读取，尝试不同宽度/类型
inline auto makeAnyIntegerRoutine(ScanMatchType matchType,
                                  bool reverseEndianness) -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        auto try_one = [&](auto tag) -> unsigned {
            using T = decltype(tag);
            if (memLength < sizeof(T)) return 0;
            T memv{};
            try {
                memv = memoryPtr->get<T>();
            } catch (...) {
                return 0;
            }
            memv = swapIfReverse<T>(memv, reverseEndianness);
            return numericMatchCore<T>(matchType, memv, oldValue, userValue,
                                       saveFlags);
        };
        // 依次尝试常见整型宽度（无符号/有符号）
        if (auto r = try_one(uint64_t{})) return r;
        if (auto r = try_one(int64_t{})) return r;
        if (auto r = try_one(uint32_t{})) return r;
        if (auto r = try_one(int32_t{})) return r;
        if (auto r = try_one(uint16_t{})) return r;
        if (auto r = try_one(int16_t{})) return r;
        if (auto r = try_one(uint8_t{})) return r;
        if (auto r = try_one(int8_t{})) return r;
        return 0;
    };
}

inline auto makeAnyFloatRoutine(ScanMatchType matchType, bool reverseEndianness)
    -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        auto try_one = [&](auto tag) -> unsigned {
            using T = decltype(tag);
            if (memLength < sizeof(T)) return 0;
            T memv{};
            try {
                memv = memoryPtr->get<T>();
            } catch (...) {
                return 0;
            }
            memv = swapIfReverse<T>(memv, reverseEndianness);
            return numericMatchCore<T>(matchType, memv, oldValue, userValue,
                                       saveFlags);
        };
        if (auto r = try_one(double{})) return r;
        if (auto r = try_one(float{})) return r;
        return 0;
    };
}

inline auto makeAnyNumberRoutine(ScanMatchType matchType,
                                 bool reverseEndianness) -> scanRoutine {
    return [matchType, reverseEndianness](
               const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
               const UserValue* userValue, MatchFlags* saveFlags) -> unsigned {
        *saveFlags = MatchFlags::EMPTY;
        // 先尝试浮点，再尝试整数（或按需调整策略）
        if (auto r = makeAnyFloatRoutine(matchType, reverseEndianness)(
                memoryPtr, memLength, oldValue, userValue, saveFlags)) {
            return r;
        }
        return makeAnyIntegerRoutine(matchType, reverseEndianness)(
            memoryPtr, memLength, oldValue, userValue, saveFlags);
    };
}

// 工厂：按数据类型返回对应的扫描例程。
// ANY* 系列目前尚未实现（需要聚合多类型尝试，后续扩展）。
export [[nodiscard]] inline auto smGetScanroutine(ScanDataType dataType,
                                                  ScanMatchType matchType,
                                                  MatchFlags /*uflags*/,
                                                  bool reverseEndianness)
    -> scanRoutine {
    switch (dataType) {
        case ScanDataType::INTEGER8:
            return makeNumericRoutine<int8_t>(matchType, reverseEndianness);
        case ScanDataType::INTEGER16:
            return makeNumericRoutine<int16_t>(matchType, reverseEndianness);
        case ScanDataType::INTEGER32:
            return makeNumericRoutine<int32_t>(matchType, reverseEndianness);
        case ScanDataType::INTEGER64:
            return makeNumericRoutine<int64_t>(matchType, reverseEndianness);
        case ScanDataType::FLOAT32:
            return makeNumericRoutine<float>(matchType, reverseEndianness);
        case ScanDataType::FLOAT64:
            return makeNumericRoutine<double>(matchType, reverseEndianness);
        case ScanDataType::BYTEARRAY:
            return makeBytearrayRoutine(matchType);
        case ScanDataType::STRING:
            return makeStringRoutine(matchType);
        case ScanDataType::ANYINTEGER:
            return makeAnyIntegerRoutine(matchType, reverseEndianness);
        case ScanDataType::ANYFLOAT:
            return makeAnyFloatRoutine(matchType, reverseEndianness);
        case ScanDataType::ANYNUMBER:
            return makeAnyNumberRoutine(matchType, reverseEndianness);
        default:
            return scanRoutine{};  // 尚未实现的聚合类型
    }
}

export inline auto smChooseScanroutine(ScanDataType dataType,
                                       ScanMatchType matchType,
                                       UserValue& userValue,
                                       bool reverseEndianness) -> bool {
    auto routine = smGetScanroutine(dataType, matchType, userValue.flags,
                                    reverseEndianness);
    return static_cast<bool>(routine);
}
