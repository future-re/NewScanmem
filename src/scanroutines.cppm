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
#include <unordered_map>
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

// 通用匹配辅助函数
template <typename T, typename Predicate>
inline auto genericMatchImpl(const T& memv, const Value* oldValue,
                             const UserValue* userValue, MatchFlags* saveFlags,
                             Predicate pred) -> unsigned int {
    if (pred(memv, oldValue, userValue)) {
        *saveFlags = flagForType<T>();
        return sizeof(T);
    }
    return 0;
}

// 小型匹配辅助函数已被通用模板 genericMatchImpl 替代。

/**
 * 构造数值类型的扫描例程
 *
 * 返回一个 scanRoutine：在给定内存位置尝试以 T 解读数据并按 matchType
 * 执行比较。 约定：当 memLength < sizeof(T) 或 memoryPtr->get<T>()
 * 抛出异常时返回 0。 当匹配成功时会设置 *saveFlags 并返回 sizeof(T)。
 */
// Predicate map for numeric match types
template <typename T>
static const auto& getNumericPredicateMap() {
    static const std::unordered_map<ScanMatchType, std::function<bool(const T&, const Value*, const UserValue*)>> pred_map = {
        {ScanMatchType::MATCHEQUALTO, [](const T& memv, const Value*, const UserValue* userValuePtr) -> bool {
            if (!userValuePtr) return false;
            T userVal = userValueAs<T>(*userValuePtr);
            return memv == userVal;
        }},
        {ScanMatchType::MATCHNOTEQUALTO, [](const T& memv, const Value*, const UserValue* userValuePtr) -> bool {
            if (!userValuePtr) return false;
            T userVal = userValueAs<T>(*userValuePtr);
            return memv != userVal;
        }},
        {ScanMatchType::MATCHGREATERTHAN, [](const T& memv, const Value*, const UserValue* userValuePtr) -> bool {
            if (!userValuePtr) return false;
            T userVal = userValueAs<T>(*userValuePtr);
            return memv > userVal;
        }},
        {ScanMatchType::MATCHLESSTHAN, [](const T& memv, const Value*, const UserValue* userValuePtr) -> bool {
            if (!userValuePtr) return false;
            T userVal = userValueAs<T>(*userValuePtr);
            return memv < userVal;
        }},
        {ScanMatchType::MATCHUPDATE, [](const T& memv, const Value* oldValuePtr, const UserValue*) -> bool {
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                return memv == *opt;
            }
            return false;
        }},
        {ScanMatchType::MATCHCHANGED, [](const T& memv, const Value* oldValuePtr, const UserValue*) -> bool {
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                return memv != *opt;
            }
            return false;
        }},
        {ScanMatchType::MATCHNOTCHANGED, [](const T& memv, const Value* oldValuePtr, const UserValue*) -> bool {
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                return memv == *opt;
            }
            return false;
        }},
        {ScanMatchType::MATCHINCREASED, [](const T& memv, const Value* oldValuePtr, const UserValue*) -> bool {
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                return memv > *opt;
            }
            return false;
        }},
        {ScanMatchType::MATCHDECREASED, [](const T& memv, const Value* oldValuePtr, const UserValue*) -> bool {
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                return memv < *opt;
            }
            return false;
        }},
        {ScanMatchType::MATCHINCREASEDBY, [](const T& memv, const Value* oldValuePtr, const UserValue* userValuePtr) -> bool {
            if (!userValuePtr) return false;
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                T userVal = userValueAs<T>(*userValuePtr);
                return memv - *opt == userVal;
            }
            return false;
        }},
        {ScanMatchType::MATCHDECREASEDBY, [](const T& memv, const Value* oldValuePtr, const UserValue* userValuePtr) -> bool {
            if (!userValuePtr) return false;
            if (auto opt = oldValueAs<T>(oldValuePtr)) {
                T userVal = userValueAs<T>(*userValuePtr);
                return *opt - memv == userVal;
            }
            return false;
        }}
    };
    return pred_map;
}

// Helper to check if a match type requires a user value
inline bool requiresUserValue(ScanMatchType matchType) {
    return matchType == ScanMatchType::MATCHEQUALTO ||
           matchType == ScanMatchType::MATCHNOTEQUALTO ||
           matchType == ScanMatchType::MATCHGREATERTHAN ||
           matchType == ScanMatchType::MATCHLESSTHAN ||
           matchType == ScanMatchType::MATCHINCREASEDBY ||
           matchType == ScanMatchType::MATCHDECREASEDBY;
}

// Helper to safely extract memory value
template <typename T>
inline auto safeGetMemoryValue(const Mem64* memoryPtr) -> std::optional<T> {
    try {
        return memoryPtr->get<T>();
    } catch (...) {
        return std::nullopt;
    }
}

// Core numeric matcher template. Returns bytes needed (sizeof T) or 0.
template <typename T>
auto makeNumericRoutine(ScanMatchType matchType, bool /*reverseEndianess*/)
    -> scanRoutine {
    return [matchType](const Mem64* memoryPtr, size_t memLength,
                       const Value* oldValue, const UserValue* userValue,
                       MatchFlags* saveFlags) -> unsigned int {
        // Check for sufficient memory
        if (memLength < sizeof(T)) {
            return 0;
        }
        
        // Initialize flags
        *saveFlags = MatchFlags::EMPTY;
        
        // Handle MATCHANY separately (snapshot)
        if (matchType == ScanMatchType::MATCHANY) {
            *saveFlags = flagForType<T>();
            return sizeof(T);
        }
        
        // Check if user value is required but missing
        if (requiresUserValue(matchType) && userValue == nullptr) {
            return 0;
        }
        
        // Extract memory value safely
        auto memvOpt = safeGetMemoryValue<T>(memoryPtr);
        if (!memvOpt) {
            return 0;
        }
        T memv = *memvOpt;
        
        // Get predicate and apply match
        const auto& predMap = getNumericPredicateMap<T>();
        auto it = predMap.find(matchType);
        if (it != predMap.end()) {
            return genericMatchImpl<T>(memv, oldValue, userValue, saveFlags, it->second);
        }
        
        return 0;
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