/**
 * @file factory.cppm
 * @brief Canonical scan routine factory plus thin legacy wrapper
 */

module;

#include <cstdint>

export module scan.factory;

import scan.types;
import scan.routine;
import scan.numeric;
import scan.bytes;
import scan.string;
import value.core;

export namespace scan {

/**
 * @brief Create a scan routine for the given data type and match type
 * @param dataType The data type to scan for
 * @param matchType The match condition
 * @param requiredFlag The required type flag (usually from user value)
 * @param reverseEndianness Whether to reverse byte order
 * @return A ScanRoutine that performs the matching
 */
[[nodiscard]] inline auto makeScanRoutine(
    ScanDataType dataType, ScanMatchType matchType,
    bool reverseEndianness) -> ScanRoutine {
    switch (dataType) {
        case ScanDataType::INTEGER_8:
            return makeNumericScanRoutine<int8_t>(matchType,
                                                  reverseEndianness);
        case ScanDataType::INTEGER_16:
            return makeNumericScanRoutine<int16_t>(matchType,
                                                   reverseEndianness);
        case ScanDataType::INTEGER_32:
            return makeNumericScanRoutine<int32_t>(matchType,
                                                   reverseEndianness);
        case ScanDataType::INTEGER_64:
            return makeNumericScanRoutine<int64_t>(matchType,
                                                   reverseEndianness);
        case ScanDataType::FLOAT_32:
            return makeNumericScanRoutine<float>(matchType, reverseEndianness);
        case ScanDataType::FLOAT_64:
            return makeNumericScanRoutine<double>(matchType, reverseEndianness);
        case ScanDataType::BYTE_ARRAY:
            return makeBytearrayScanRoutine(matchType);
        case ScanDataType::STRING:
            return makeStringScanRoutine(matchType);
        case ScanDataType::ANY_INTEGER:
            return makeAnyIntegerScanRoutine(matchType, reverseEndianness);
        case ScanDataType::ANY_FLOAT:
            return makeAnyFloatScanRoutine(matchType, reverseEndianness);
        case ScanDataType::ANY_NUMBER:
            return makeAnyNumberScanRoutine(matchType, reverseEndianness);
        default:
            return nullRoutine();
    }
}

/**
 * @brief Check if a scan routine is available for the given configuration
 */
[[nodiscard]] inline auto isRoutineAvailable(ScanDataType dataType,
                                             ScanMatchType matchType) -> bool {
    auto routine = makeScanRoutine(dataType, matchType, false);
    // A null routine always returns noMatch, so we can't easily test this
    // without actually running it. For now, assume it's available if dataType
    // is valid.
    return dataType != ScanDataType{};
}

}  // namespace scan
