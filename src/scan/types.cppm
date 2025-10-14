module;

#include <cstddef>
#include <cstdint>
#include <functional>

export module scan.types;

// 轻量级前置声明：避免在基础类型定义阶段引入重依赖
export struct Mem64;      // 来自 mem64 模块
export struct Value;      // 旧版/快照值（后续由 value 模块提供）
export struct UserValue;  // 旧版用户输入（逐步被 UserInput 取代）
export enum class MatchFlags : std::uint16_t;  // 来自 value.flags

// 扫描数据类型分类
export enum class ScanDataType {
    ANYNUMBER,   // 匹配任何数值类型（整数或浮点）
    ANYINTEGER,  // 匹配任何整数类型（8/16/32/64 位）
    ANYFLOAT,    // 匹配任何浮点类型（32/64 位）
    INTEGER8,    // 8 位整数
    INTEGER16,   // 16 位整数
    INTEGER32,   // 32 位整数
    INTEGER64,   // 64 位整数
    FLOAT32,     // 32 位浮点
    FLOAT64,     // 64 位浮点
    BYTEARRAY,   // 字节数组
    STRING       // 字符串
};

// 匹配方式分类
export enum class ScanMatchType {
    // 快照类（不依赖用户输入数值）
    MATCHANY,          // 匹配任何值（记录快照）
    MATCHUPDATE,       // 与旧值相等
    MATCHNOTCHANGED,   // 与旧值相等（与 MATCHUPDATE 等价）
    MATCHCHANGED,      // 与旧值不等
    MATCHINCREASED,    // 大于旧值
    MATCHDECREASED,    // 小于旧值
    MATCHINCREASEDBY,  // 当前值 - 旧值 == 用户值
    MATCHDECREASEDBY,  // 旧值 - 当前值 == 用户值

    // 与用户提供的值比较（需提供用户值）
    MATCHEQUALTO,      // 等于
    MATCHNOTEQUALTO,   // 不等于
    MATCHGREATERTHAN,  // 大于
    MATCHLESSTHAN,     // 小于
    MATCHRANGE,        // 范围 [low, high]
    MATCHREGEX         // 正则（STRING 专用）
};

// 单个扫描位置的匹配函数签名。返回匹配的字节数（>=1）或 0（未匹配）。
export using scanRoutine = std::function<unsigned int(
    const Mem64* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;

// 字节模式查找结果（带偏移与长度），在 targetmem 或范围标记时有用
export struct ByteMatch {
    size_t offset{};
    size_t length{};
};

// 常用的小工具（仅类型层面的便捷判定，不引入实现依赖）
export constexpr auto isNumericType(ScanDataType scanDataType) noexcept
    -> bool {
    switch (scanDataType) {
        case ScanDataType::INTEGER8:
        case ScanDataType::INTEGER16:
        case ScanDataType::INTEGER32:
        case ScanDataType::INTEGER64:
        case ScanDataType::FLOAT32:
        case ScanDataType::FLOAT64:
        case ScanDataType::ANYINTEGER:
        case ScanDataType::ANYFLOAT:
        case ScanDataType::ANYNUMBER:
            return true;
        case ScanDataType::BYTEARRAY:
        case ScanDataType::STRING:
            return false;
    }
    return false;
}

export constexpr auto isAggregatedAny(ScanDataType scanDataType) noexcept
    -> bool {
    return scanDataType == ScanDataType::ANYNUMBER ||
           scanDataType == ScanDataType::ANYINTEGER ||
           scanDataType == ScanDataType::ANYFLOAT;
}

export constexpr auto matchNeedsUserValue(ScanMatchType scanMatchType) noexcept
    -> bool {
    switch (scanMatchType) {
        case ScanMatchType::MATCHEQUALTO:
        case ScanMatchType::MATCHNOTEQUALTO:
        case ScanMatchType::MATCHGREATERTHAN:
        case ScanMatchType::MATCHLESSTHAN:
        case ScanMatchType::MATCHRANGE:
        case ScanMatchType::MATCHREGEX:
        case ScanMatchType::MATCHINCREASEDBY:
        case ScanMatchType::MATCHDECREASEDBY:
            return true;
        default:
            return false;
    }
}

export constexpr auto matchUsesOldValue(ScanMatchType scanMatchType) noexcept
    -> bool {
    switch (scanMatchType) {
        case ScanMatchType::MATCHUPDATE:
        case ScanMatchType::MATCHNOTCHANGED:
        case ScanMatchType::MATCHCHANGED:
        case ScanMatchType::MATCHINCREASED:
        case ScanMatchType::MATCHDECREASED:
        case ScanMatchType::MATCHINCREASEDBY:
        case ScanMatchType::MATCHDECREASEDBY:
            return true;
        default:
            return false;
    }
}
