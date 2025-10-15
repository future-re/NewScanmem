module;

#include <cstdint>

export module scan.factory;

import scan.types;
import scan.numeric;
import scan.bytes;
import scan.string;
import value;

// 本模块提供统一的工厂入口：smGetScanroutine / smChooseScanroutine
// 它将根据 ScanDataType 返回合适的 scanRoutine。

export [[nodiscard]] inline auto smGetScanroutine(ScanDataType dataType,
                                                  ScanMatchType matchType,
                                                  [[maybe_unused]] MatchFlags uflags,
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
            return scanRoutine{};
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
