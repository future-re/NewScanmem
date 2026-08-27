#include "newscanmem/scan/factory.hpp"

namespace scan {
auto makeScanRoutine(const ScanDataType type, const ScanMatchType match,
                     const bool reverseEndianness) -> ScanRoutine {
    switch (type) {
        case ScanDataType::INTEGER_8:
            return makeNumericScanRoutine<std::int8_t>(match,
                                                       reverseEndianness);
        case ScanDataType::INTEGER_16:
            return makeNumericScanRoutine<std::int16_t>(match,
                                                        reverseEndianness);
        case ScanDataType::INTEGER_32:
            return makeNumericScanRoutine<std::int32_t>(match,
                                                        reverseEndianness);
        case ScanDataType::INTEGER_64:
            return makeNumericScanRoutine<std::int64_t>(match,
                                                        reverseEndianness);
        case ScanDataType::FLOAT_32:
            return makeNumericScanRoutine<float>(match, reverseEndianness);
        case ScanDataType::FLOAT_64:
            return makeNumericScanRoutine<double>(match, reverseEndianness);
        case ScanDataType::BYTE_ARRAY:
            return makeBytearrayScanRoutine(match);
        case ScanDataType::STRING:
            return makeStringScanRoutine(match);
        case ScanDataType::ANY_INTEGER:
            return makeAnyIntegerScanRoutine(match, reverseEndianness);
        case ScanDataType::ANY_FLOAT:
            return makeAnyFloatScanRoutine(match, reverseEndianness);
        case ScanDataType::ANY_NUMBER:
            return makeAnyNumberScanRoutine(match, reverseEndianness);
        default:
            return nullRoutine();
    }
}
auto isRoutineAvailable(const ScanDataType type, const ScanMatchType) -> bool {
    return type != ScanDataType{};
}
}  // namespace scan
