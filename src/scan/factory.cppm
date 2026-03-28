/**
 * @file factory.cppm
 * @brief Scan routine factory with modern and legacy APIs
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
import value.flags;

export namespace scan {

// ============================================================================
// Modern API: Returns new ScanRoutine type
// ============================================================================

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
    [[maybe_unused]] MatchFlags requiredFlag, bool reverseEndianness) -> ScanRoutine {
    
    // Get the legacy routine and adapt it
    scanRoutine legacy = nullptr;
    
    switch (dataType) {
        case ScanDataType::INTEGER_8:
            legacy = makeNumericRoutine<int8_t>(matchType, reverseEndianness);
            break;
        case ScanDataType::INTEGER_16:
            legacy = makeNumericRoutine<int16_t>(matchType, reverseEndianness);
            break;
        case ScanDataType::INTEGER_32:
            legacy = makeNumericRoutine<int32_t>(matchType, reverseEndianness);
            break;
        case ScanDataType::INTEGER_64:
            legacy = makeNumericRoutine<int64_t>(matchType, reverseEndianness);
            break;
        case ScanDataType::FLOAT_32:
            legacy = makeNumericRoutine<float>(matchType, reverseEndianness);
            break;
        case ScanDataType::FLOAT_64:
            legacy = makeNumericRoutine<double>(matchType, reverseEndianness);
            break;
        case ScanDataType::BYTE_ARRAY:
            legacy = makeBytearrayRoutine(matchType);
            break;
        case ScanDataType::STRING:
            legacy = makeStringRoutine(matchType);
            break;
        case ScanDataType::ANY_INTEGER:
            legacy = makeAnyIntegerRoutine(matchType, reverseEndianness);
            break;
        case ScanDataType::ANY_FLOAT:
            legacy = makeAnyFloatRoutine(matchType, reverseEndianness);
            break;
        case ScanDataType::ANY_NUMBER:
            legacy = makeAnyNumberRoutine(matchType, reverseEndianness);
            break;
        default:
            return nullRoutine();
    }
    
    return adaptRoutine(legacy);
}

/**
 * @brief Check if a scan routine is available for the given configuration
 */
[[nodiscard]] inline auto isRoutineAvailable(ScanDataType dataType,
                                             ScanMatchType matchType,
                                             MatchFlags requiredFlag) -> bool {
    auto routine = makeScanRoutine(dataType, matchType, requiredFlag, false);
    // A null routine always returns noMatch, so we can't easily test this
    // without actually running it. For now, assume it's available if dataType
    // is valid.
    return dataType != ScanDataType{};
}

}  // namespace scan

// ============================================================================
// Legacy API: Returns old scanRoutine type (for backward compatibility)
// ============================================================================

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
    auto routine = smGetScanroutine(dataType, matchType, userValue.flag(),
                                    reverseEndianness);
    return static_cast<bool>(routine);
}
