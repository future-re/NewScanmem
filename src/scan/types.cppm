module;

#include <cstddef>
#include <functional>

export module scan.types;

import value; // 依赖仓库已有的 value 模块，用于 Value/UserValue/MatchFlags 等定义

export enum class ScanDataType {
    ANYNUMBER,
    ANYINTEGER,
    ANYFLOAT,
    INTEGER8,
    INTEGER16,
    INTEGER32,
    INTEGER64,
    FLOAT32,
    FLOAT64,
    BYTEARRAY,
    STRING
};

export enum class ScanMatchType {
    MATCHANY,
    MATCHEQUALTO,
    MATCHNOTEQUALTO,
    MATCHGREATERTHAN,
    MATCHLESSTHAN,
    MATCHRANGE,
    MATCHREGEX,
    MATCHUPDATE,
    MATCHNOTCHANGED,
    MATCHCHANGED,
    MATCHINCREASED,
    MATCHDECREASED,
    MATCHINCREASEDBY,
    MATCHDECREASEDBY
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
