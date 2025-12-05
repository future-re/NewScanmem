module;

#include <cstdint>

export module scan.factory;

import scan.types;
import scan.numeric;
import scan.bytes;
import scan.string;
import value;

// This module provides a unified factory entry: smGetScanroutine /
// smChooseScanroutine It returns an appropriate scanRoutine given a
// ScanDataType.

export [[nodiscard]] inline auto smGetScanroutine(
    ScanDataType dataType, ScanMatchType matchType,
    [[maybe_unused]] MatchFlags uflags, bool reverseEndianness) -> scanRoutine {
    switch (dataType) {
        case ScanDataType::INTEGER_8:
            return makeNumericRoutine<int8_t>(matchType, reverseEndianness);
        case ScanDataType::INTEGER_16:
            return makeNumericRoutine<int16_t>(matchType, reverseEndianness);
        case ScanDataType::INTEGER_32:
            return makeNumericRoutine<int32_t>(matchType, reverseEndianness);
        case ScanDataType::INTEGER_64:
            return makeNumericRoutine<int64_t>(matchType, reverseEndianness);
        case ScanDataType::FLOAT_32:
            return makeNumericRoutine<float>(matchType, reverseEndianness);
        case ScanDataType::FLOAT_64:
            return makeNumericRoutine<double>(matchType, reverseEndianness);
        case ScanDataType::BYTE_ARRAY:
            return makeBytearrayRoutine(matchType);
        case ScanDataType::STRING:
            return makeStringRoutine(matchType);
        case ScanDataType::ANY_INTEGER:
            return makeAnyIntegerRoutine(matchType, reverseEndianness);
        case ScanDataType::ANY_FLOAT:
            return makeAnyFloatRoutine(matchType, reverseEndianness);
        case ScanDataType::ANY_NUMBER:
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
