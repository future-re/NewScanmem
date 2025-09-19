module;

#include <cstddef>
#include <functional>

export module scan.types;

import value; // 依赖仓库已有的 value 模块，用于 Value/UserValue/MatchFlags 等定义

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

export enum class ScanMatchType {
    // 相对于UserValue
    MATCHANY,          // 匹配任何值
    MATCHEQUALTO,      // 等于
    MATCHNOTEQUALTO,   // 不等于
    MATCHGREATERTHAN,  // 大于
    MATCHLESSTHAN,     // 小于
    MATCHRANGE,        // 范围
    MATCHREGEX,        // 正则表达式
    MATCHUPDATE,       // 更新
    MATCHNOTCHANGED,   // 未更改
    // 相对于Value
    MATCHCHANGED,      // 已更改
    MATCHINCREASED,    // 增加，
    MATCHDECREASED,    // 减少
    MATCHINCREASEDBY,  // 增加指定值
    MATCHDECREASEDBY   // 减少指定值
};

// 单个扫描位置的匹配函数签名。返回匹配的字节数（>=1）或 0（未匹配）。
export using scanRoutine = std::function<unsigned int(
    const Mem64* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;

// 字节模式查找结果（带偏移与长度），在 targetmem 或范围标记时有用
export struct ByteMatch {
    size_t offset;
    size_t length;
};

// 未来可以在此处扩展导出常用别名或小工具
