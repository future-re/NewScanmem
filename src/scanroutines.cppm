module;

#include <array>
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
enum class ScanDataType {
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
enum class ScanMatchType {
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
using scanRoutine = std::function<unsigned int(
    const Mem64* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;

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

// 小型匹配辅助函数
// 这些 helper 将常见的比较逻辑封装成统一签名，以降低主匹配 lambda
// 的认知复杂度。
template <typename T>
inline auto matchEqualImpl(const T& memv, const Value* /*oldValue*/,
                           const UserValue* userValue, MatchFlags* saveFlags)
    -> unsigned int {
    T userValT = userValueAs<T>(*userValue);
    if (memv == userValT) {
        *saveFlags = flagForType<T>();
        return sizeof(T);
    }
    return 0;
}

template <typename T>
inline auto matchNotEqualImpl(const T& memv, const Value* /*oldValue*/,
                              const UserValue* userValue, MatchFlags* saveFlags)
    -> unsigned int {
    T userValT = userValueAs<T>(*userValue);
    if (memv != userValT) {
        *saveFlags = flagForType<T>();
        return sizeof(T);
    }
    return 0;
}

template <typename T>
inline auto matchGtImpl(const T& memv, const Value* /*oldValue*/,
                        const UserValue* userValue, MatchFlags* saveFlags)
    -> unsigned int {
    T userValT = userValueAs<T>(*userValue);
    if (memv > userValT) {
        *saveFlags = flagForType<T>();
        return sizeof(T);
    }
    return 0;
}

template <typename T>
inline auto matchLtImpl(const T& memv, const Value* /*oldValue*/,
                        const UserValue* userValue, MatchFlags* saveFlags)
    -> unsigned int {
    T userValT = userValueAs<T>(*userValue);
    if (memv < userValT) {
        *saveFlags = flagForType<T>();
        return sizeof(T);
    }
    return 0;
}

template <typename T>
inline auto matchUpdateImpl(const T& memv, const Value* oldValue,
                            const UserValue* /*userValue*/,
                            MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        if (memv == *oldOpt) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

template <typename T>
inline auto matchChangedImpl(const T& memv, const Value* oldValue,
                             const UserValue* /*userValue*/,
                             MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        if (memv != *oldOpt) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

template <typename T>
inline auto matchNotChangedImpl(const T& memv, const Value* oldValue,
                                const UserValue* /*userValue*/,
                                MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        if (memv == *oldOpt) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

template <typename T>
inline auto matchIncreasedImpl(const T& memv, const Value* oldValue,
                               const UserValue* /*userValue*/,
                               MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        if (memv > *oldOpt) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

template <typename T>
inline auto matchDecreasedImpl(const T& memv, const Value* oldValue,
                               const UserValue* /*userValue*/,
                               MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        if (memv < *oldOpt) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

template <typename T>
inline auto matchIncreasedByImpl(const T& memv, const Value* oldValue,
                                 const UserValue* userValue,
                                 MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        T userValT = userValueAs<T>(*userValue);
        if (memv - *oldOpt == userValT) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

template <typename T>
inline auto matchDecreasedByImpl(const T& memv, const Value* oldValue,
                                 const UserValue* userValue,
                                 MatchFlags* saveFlags) -> unsigned int {
    if (auto oldOpt = oldValueAs<T>(oldValue)) {
        T userValT = userValueAs<T>(*userValue);
        if (*oldOpt - memv == userValT) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
    }
    return 0;
}

/**
 * 构造数值类型的扫描例程
 *
 * 返回一个 scanRoutine：在给定内存位置尝试以 T 解读数据并按 matchType
 * 执行比较。 约定：当 memLength < sizeof(T) 或 memoryPtr->get<T>()
 * 抛出异常时返回 0。 当匹配成功时会设置 *saveFlags 并返回 sizeof(T)。
 */
// Core numeric matcher template. Returns bytes needed (sizeof T) or 0.
template <typename T>
auto makeNumericRoutine(ScanMatchType matchType, bool /*reverseEndianess*/)
    -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* oldValue, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        if (memLength < sizeof(T)) {
            return 0;
        }
        // extract memory value
        T memv;
        try {
            memv = memoryPtr->get<T>();
        } catch (...) {
            return 0;
        }

        *saveFlags = MatchFlags::EMPTY;

        // Guard: if match type requires a provided user value but none is
        // supplied, bail out early.
        if ((matchType == ScanMatchType::MATCHEQUALTO ||
             matchType == ScanMatchType::MATCHNOTEQUALTO ||
             matchType == ScanMatchType::MATCHGREATERTHAN ||
             matchType == ScanMatchType::MATCHLESSTHAN ||
             matchType == ScanMatchType::MATCHINCREASEDBY ||
             matchType == ScanMatchType::MATCHDECREASEDBY) &&
            (userValue == nullptr)) {
            return 0;
        }

        switch (matchType) {
            case ScanMatchType::MATCHANY:
                // match by default (snapshot)
                *saveFlags = flagForType<T>();
                return sizeof(T);

            case ScanMatchType::MATCHEQUALTO:
                return matchEqualImpl<T>(memv, oldValue, userValue, saveFlags);

            case ScanMatchType::MATCHNOTEQUALTO:
                return matchNotEqualImpl<T>(memv, oldValue, userValue,
                                            saveFlags);

            case ScanMatchType::MATCHGREATERTHAN:
                return matchGtImpl<T>(memv, oldValue, userValue, saveFlags);

            case ScanMatchType::MATCHLESSTHAN:
                return matchLtImpl<T>(memv, oldValue, userValue, saveFlags);

            case ScanMatchType::MATCHUPDATE:
                return matchUpdateImpl<T>(memv, oldValue, userValue, saveFlags);

            case ScanMatchType::MATCHCHANGED:
                return matchChangedImpl<T>(memv, oldValue, userValue,
                                           saveFlags);

            case ScanMatchType::MATCHNOTCHANGED:
                return matchNotChangedImpl<T>(memv, oldValue, userValue,
                                              saveFlags);

            case ScanMatchType::MATCHINCREASED:
                return matchIncreasedImpl<T>(memv, oldValue, userValue,
                                             saveFlags);

            case ScanMatchType::MATCHDECREASED:
                return matchDecreasedImpl<T>(memv, oldValue, userValue,
                                             saveFlags);

            case ScanMatchType::MATCHINCREASEDBY:
                return matchIncreasedByImpl<T>(memv, oldValue, userValue,
                                               saveFlags);

            case ScanMatchType::MATCHDECREASEDBY:
                return matchDecreasedByImpl<T>(memv, oldValue, userValue,
                                               saveFlags);

            default:
                return 0;
        }
    };
}

// 字节数组 / 字符串 的匹配器工厂
// 注意：当前实现对任意长度的支持受限（使用 Mem64 的固定数组变体），
// 应在将来改为从目标内存读取任意长度缓冲区再比较。
inline auto makeBytearrayRoutine(ScanMatchType matchType) -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* /*oldValue*/, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        *saveFlags = MatchFlags::EMPTY;
        if (matchType == ScanMatchType::MATCHANY) {
            return static_cast<unsigned int>(memLength);
        }
        if (!userValue->bytearrayValue) {
            return 0;
        }
        const auto& byteArray = *userValue->bytearrayValue;
        if (matchType == ScanMatchType::MATCHEQUALTO) {
            if (byteArray.size() != memLength) {
                return 0;
            }
            try {
                auto memArr =
                    memoryPtr->get<std::array<uint8_t, sizeof(int64_t)>>();
                if (::memcmp(memArr.data(), byteArray.data(), memLength) != 0) {
                    return 0;
                }
                *saveFlags = MatchFlags::B8;  // generic byte flag
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
        const auto& str = userValue->stringValue;
        if (matchType == ScanMatchType::MATCHEQUALTO) {
            if (str.size() != memLength) {
                return 0;
            }
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

// 前向声明：按数据类型和匹配类型返回相应的 scanRoutine（实现位于文件下方）
auto smGetScanroutine(ScanDataType dataType, ScanMatchType matchType,
                      MatchFlags uflags, bool reverseEndianness) -> scanRoutine;

auto smChooseScanroutine(ScanDataType dataType, ScanMatchType matchType,
                         UserValue& userValue, bool reverseEndianness) -> bool {
    // 选择并验证是否存在对应的扫描例程
    auto routine = smGetScanroutine(dataType, matchType, userValue.flags,
                                    reverseEndianness);
    return static_cast<bool>(routine);
}

auto smGetScanroutine(ScanDataType dataType, ScanMatchType matchType,
                      MatchFlags uflags, bool reverseEndianness)
    -> scanRoutine {
    (void)uflags;             // 目前未使用的用户标志，保留以备将来验证/过滤
    (void)reverseEndianness;  // 目前未实现字节序翻转，参数占位
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
        case ScanDataType::ANYNUMBER:
        case ScanDataType::ANYINTEGER:
        case ScanDataType::ANYFLOAT:
        default:
            return scanRoutine{};  // empty
    }
}