module;

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

export module scanroutines;

import value;

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

// Helpers: map C++ type -> MatchFlags flag for the size/type
//
// 说明：根据模板类型 T 返回对应的 MatchFlags，用于在匹配时记录匹配的类型/宽度。
template <typename T>
constexpr auto flagForType() noexcept -> MatchFlags {
    if constexpr (std::is_same_v<T, int8_t>) {
        return MatchFlags::S8B;
    }
    if constexpr (std::is_same_v<T, uint8_t>) {
        return MatchFlags::U8B;
    }
    if constexpr (std::is_same_v<T, int16_t>) {
        return MatchFlags::S16B;
    }
    if constexpr (std::is_same_v<T, uint16_t>) {
        return MatchFlags::U16B;
    }
    if constexpr (std::is_same_v<T, int32_t>) {
        return MatchFlags::S32B;
    }
    if constexpr (std::is_same_v<T, uint32_t>) {
        return MatchFlags::U32B;
    }
    if constexpr (std::is_same_v<T, int64_t>) {
        return MatchFlags::S64B;
    }
    if constexpr (std::is_same_v<T, uint64_t>) {
        return MatchFlags::U64B;
    }
    if constexpr (std::is_same_v<T, float>) {
        return MatchFlags::F32B;
    }
    if constexpr (std::is_same_v<T, double>) {
        return MatchFlags::F64B;
    }
    return MatchFlags::EMPTY;
}

// Extract user value for given type T from UserValue
/**
 * 从 UserValue 中按模板类型提取用户值
 *
 * 注意：调用方应确保 userValue 已经存在且与 matchType 要求一致。
 */
template <typename T>
inline auto userValueAs(const UserValue& userVal) -> T {
    if constexpr (std::is_same_v<T, int8_t>) {
        return userVal.int8Value;
    }
    if constexpr (std::is_same_v<T, uint8_t>) {
        return userVal.uint8Value;
    }
    if constexpr (std::is_same_v<T, int16_t>) {
        return userVal.int16Value;
    }
    if constexpr (std::is_same_v<T, uint16_t>) {
        return userVal.uint16Value;
    }
    if constexpr (std::is_same_v<T, int32_t>) {
        return userVal.int32Value;
    }
    if constexpr (std::is_same_v<T, uint32_t>) {
        return userVal.uint32Value;
    }
    if constexpr (std::is_same_v<T, int64_t>) {
        return userVal.int64Value;
    }
    if constexpr (std::is_same_v<T, uint64_t>) {
        return userVal.uint64Value;
    }
    if constexpr (std::is_same_v<T, float>) {
        return userVal.float32Value;
    }
    if constexpr (std::is_same_v<T, double>) {
        return userVal.float64Value;
    }
    // 应该不会到达这里，但为了编译安全
    static_assert(std::is_arithmetic_v<T>, "Unsupported type for userValueAs");
    return T{};
}

// 获取上界（用于 MATCHRANGE）
template <typename T>
inline auto userValueHighAs(const UserValue& userVal) -> T {
    if constexpr (std::is_same_v<T, int8_t>) {
        return userVal.int8RangeHighValue;
    }
    if constexpr (std::is_same_v<T, uint8_t>) {
        return userVal.uint8RangeHighValue;
    }
    if constexpr (std::is_same_v<T, int16_t>) {
        return userVal.int16RangeHighValue;
    }
    if constexpr (std::is_same_v<T, uint16_t>) {
        return userVal.uint16RangeHighValue;
    }
    if constexpr (std::is_same_v<T, int32_t>) {
        return userVal.int32RangeHighValue;
    }
    if constexpr (std::is_same_v<T, uint32_t>) {
        return userVal.uint32RangeHighValue;
    }
    if constexpr (std::is_same_v<T, int64_t>) {
        return userVal.int64RangeHighValue;
    }
    if constexpr (std::is_same_v<T, uint64_t>) {
        return userVal.uint64RangeHighValue;
    }
    if constexpr (std::is_same_v<T, float>) {
        return userVal.float32RangeHighValue;
    }
    if constexpr (std::is_same_v<T, double>) {
        return userVal.float64RangeHighValue;
    }
    // 应该不会到达这里，但为了编译安全
    static_assert(std::is_arithmetic_v<T>,
                  "Unsupported type for userValueHighAs");
    return T{};
}

// Try to get old value of type T from Value*
/**
 * 尝试从 Value* 中按模板类型读取先前快照值
 *
 * 返回：存在则返回 std::optional 包裹的值，否者返回 std::nullopt。
 */
template <typename T>
inline auto oldValueAs(const Value* valuePtr) -> std::optional<T> {
    if (!valuePtr) {
        return std::nullopt;
    }
    if (auto* ptr = std::get_if<T>(&valuePtr->value)) {
        return *ptr;
    }
    return std::nullopt;
}

// 统一的数值匹配核心：依据 matchType 对单个内存值做判定
// 返回匹配成功时的 sizeof(T)，否则 0。
template <typename T>
inline auto numericMatchCore(ScanMatchType matchType, T memv,
                             const Value* oldValue, const UserValue* userValue,
                             MatchFlags* saveFlags) -> unsigned int {
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
    switch (matchType) {
        case ScanMatchType::MATCHANY:
            isMatched = true;  // snapshot 收集
            break;
        case ScanMatchType::MATCHEQUALTO:
            isMatched = (memv == getUser());
            break;
        case ScanMatchType::MATCHNOTEQUALTO:
            isMatched = (memv != getUser());
            break;
        case ScanMatchType::MATCHGREATERTHAN:
            isMatched = (memv > getUser());
            break;
        case ScanMatchType::MATCHLESSTHAN:
            isMatched = (memv < getUser());
            break;
        case ScanMatchType::MATCHUPDATE:  // 与 NOTCHANGED 归并
        case ScanMatchType::MATCHNOTCHANGED:
            if (oldOpt) {
                isMatched = (memv == *oldOpt);
            }
            break;
        case ScanMatchType::MATCHCHANGED:
            if (oldOpt) {
                isMatched = (memv != *oldOpt);
            }
            break;
        case ScanMatchType::MATCHINCREASED:
            if (oldOpt) {
                isMatched = (memv > *oldOpt);
            }
            break;
        case ScanMatchType::MATCHDECREASED:
            if (oldOpt) {
                isMatched = (memv < *oldOpt);
            }
            break;
        case ScanMatchType::MATCHINCREASEDBY:
            if (oldOpt) {
                isMatched = (memv - *oldOpt == getUser());
            }
            break;
        case ScanMatchType::MATCHDECREASEDBY:
            if (oldOpt) {
                isMatched = (*oldOpt - memv == getUser());
            }
            break;
        case ScanMatchType::MATCHRANGE: {
            if (userValue) {
                T low = userValueAs<T>(*userValue);
                T high = userValueHighAs<T>(*userValue);
                if (low <= high) {
                    isMatched = (memv >= low && memv <= high);
                } else {  // 允许调用方传反，自动纠正
                    isMatched = (memv >= high && memv <= low);
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
inline auto makeNumericRoutine(ScanMatchType matchType, bool reverseEndianess)
    -> scanRoutine {
    return
        [matchType, reverseEndianess](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            if (memLength < sizeof(T)) {
                return 0;
            }
            T memv;
            try {
                memv = memoryPtr->get<T>();
            } catch (...) {
                return 0;
            }
            memv = swapIfReverse<T>(memv, reverseEndianess);
            *saveFlags = MatchFlags::EMPTY;
            return numericMatchCore<T>(matchType, memv, oldValue, userValue,
                                       saveFlags);
        };
}

// // 字节数组 / 字符串 的匹配器工厂
// // 注意：当前实现对任意长度的支持受限（使用 Mem64 的固定数组变体），
// // 应在将来改为从目标内存读取任意长度缓冲区再比较。
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
        const auto& byteArray = *userValue->bytearrayValue;
        if (matchType == ScanMatchType::MATCHEQUALTO) {
            if (byteArray.size() != memLength) {
                return 0;
            }
            if (memLength > sizeof(int64_t)) {
                return 0;
            }  // 受限实现
            try {
                auto memArr =
                    memoryPtr->get<std::array<uint8_t, sizeof(int64_t)>>();
                if (::memcmp(memArr.data(), byteArray.data(), memLength) != 0) {
                    return 0;
                }
                *saveFlags = MatchFlags::B8;  // 使用字节宽度标志
                return static_cast<unsigned int>(memLength);
            } catch (...) {
                return 0;
            }
        }
        return 0;
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
        const auto& str = userValue->stringValue;
        if (matchType == ScanMatchType::MATCHEQUALTO) {
            if (str.size() != memLength) {
                return 0;
            }
            if (memLength > sizeof(int64_t)) {
                return 0;
            }  // 受限实现
            try {
                auto memArr =
                    memoryPtr->get<std::array<char, sizeof(int64_t)>>();
                if (::memcmp(memArr.data(), str.data(), memLength) != 0) {
                    return 0;
                }
                *saveFlags = MatchFlags::B8;
                return static_cast<unsigned int>(memLength);
            } catch (...) {
                return 0;
            }
        }
        return 0;
    };
}

// ANY* 聚合例程：逐类型尝试，首个匹配即返回
// 注意：当前实现基于 Mem64::get<T>()
// 的异常路径尝试不同类型，效率一般，后续可通过 统一原始字节缓存避免多次 variant
// 访问失败抛异常。
template <typename... Ts>
struct TypeList {};

// 尝试单一类型匹配（辅助）
template <typename T>
inline unsigned int tryOneType(ScanMatchType matchType, bool reverseEndianness,
                               const Mem64* memoryPtr, size_t memLength,
                               const Value* oldValue,
                               const UserValue* userValue,
                               MatchFlags* saveFlags) {
    if (memLength < sizeof(T)) {
        return 0;
    }
    T memv;
    try {
        memv = memoryPtr->get<T>();
    } catch (...) {
        return 0;  // variant 当前不含该类型
    }
    memv = swapIfReverse<T>(memv, reverseEndianness);
    return numericMatchCore<T>(matchType, memv, oldValue, userValue, saveFlags);
}

// 递归展开 TypeList
template <typename... Ts>
inline unsigned int tryTypes(TypeList<Ts...> /*list*/, ScanMatchType matchType,
                             bool reverseEndianess, const Mem64* memoryPtr,
                             size_t memLength, const Value* oldValue,
                             const UserValue* userValue,
                             MatchFlags* saveFlags) {
    (void)matchType;
    (void)reverseEndianess;
    (void)memoryPtr;
    (void)memLength;
    (void)oldValue;
    (void)userValue;
    (void)saveFlags;
    return 0;  // 由递归重载处理非空类型列表
}

template <typename T, typename... Rest>
inline unsigned int tryTypes(TypeList<T, Rest...> /*list*/,
                             ScanMatchType matchType, bool reverseEndianess,
                             const Mem64* memoryPtr, size_t memLength,
                             const Value* oldValue, const UserValue* userValue,
                             MatchFlags* saveFlags) {
    if (auto matched =
            tryOneType<T>(matchType, reverseEndianess, memoryPtr, memLength,
                          oldValue, userValue, saveFlags)) {
        return matched;
    }
    return tryTypes(TypeList<Rest...>{}, matchType, reverseEndianess, memoryPtr,
                    memLength, oldValue, userValue, saveFlags);
}

template <>
inline unsigned int tryTypes(TypeList<>, ScanMatchType matchType,
                             bool reverseEndianess, const Mem64* memoryPtr,
                             size_t memLength, const Value* oldValue,
                             const UserValue* userValue,
                             MatchFlags* saveFlags) {
    (void)matchType;
    (void)reverseEndianess;
    (void)memoryPtr;
    (void)memLength;
    (void)oldValue;
    (void)userValue;
    (void)saveFlags;
    return 0;
}

inline auto makeAnyIntegerRoutine(ScanMatchType matchType,
                                  bool reverseEndianness) -> scanRoutine {
    using Integers = TypeList<int8_t, uint8_t, int16_t, uint16_t, int32_t,
                              uint32_t, int64_t, uint64_t>;
    return
        [matchType, reverseEndianness](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            *saveFlags = MatchFlags::EMPTY;
            return tryTypes(Integers{}, matchType, reverseEndianness, memoryPtr,
                            memLength, oldValue, userValue, saveFlags);
        };
}

inline auto makeAnyFloatRoutine(ScanMatchType matchType, bool reverseEndianness)
    -> scanRoutine {
    using Floats = TypeList<float, double>;
    return
        [matchType, reverseEndianness](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            *saveFlags = MatchFlags::EMPTY;
            return tryTypes(Floats{}, matchType, reverseEndianness, memoryPtr,
                            memLength, oldValue, userValue, saveFlags);
        };
}

inline auto makeAnyNumberRoutine(ScanMatchType matchType,
                                 bool reverseEndianness) -> scanRoutine {
    using Numbers = TypeList<int8_t, uint8_t, int16_t, uint16_t, int32_t,
                             uint32_t, int64_t, uint64_t, float, double>;
    return
        [matchType, reverseEndianness](
            const Mem64* memoryPtr, size_t memLength, const Value* oldValue,
            const UserValue* userValue, MatchFlags* saveFlags) -> unsigned int {
            *saveFlags = MatchFlags::EMPTY;
            return tryTypes(Numbers{}, matchType, reverseEndianness, memoryPtr,
                            memLength, oldValue, userValue, saveFlags);
        };
}

// 工厂：按数据类型返回对应的扫描例程。
// ANY* 系列目前尚未实现（需要聚合多类型尝试，后续扩展）。
export inline auto smGetScanroutine(ScanDataType dataType,
                                    ScanMatchType matchType,
                                    MatchFlags /*uflags*/,
                                    bool reverseEndianness) -> scanRoutine {
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