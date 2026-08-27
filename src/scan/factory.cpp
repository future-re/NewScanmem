#include "newscanmem/scan/factory.hpp"

namespace scan {
auto makeScanRoutine(const ScanDataType type, const ScanMatchType match, const bool reverse_endianness) -> ScanRoutine {
  switch (type) {
    case ScanDataType::INTEGER_8: return makeNumericScanRoutine<std::int8_t>(match, reverse_endianness);
    case ScanDataType::INTEGER_16: return makeNumericScanRoutine<std::int16_t>(match, reverse_endianness);
    case ScanDataType::INTEGER_32: return makeNumericScanRoutine<std::int32_t>(match, reverse_endianness);
    case ScanDataType::INTEGER_64: return makeNumericScanRoutine<std::int64_t>(match, reverse_endianness);
    case ScanDataType::FLOAT_32: return makeNumericScanRoutine<float>(match, reverse_endianness);
    case ScanDataType::FLOAT_64: return makeNumericScanRoutine<double>(match, reverse_endianness);
    case ScanDataType::BYTE_ARRAY: return makeBytearrayScanRoutine(match);
    case ScanDataType::STRING: return makeStringScanRoutine(match);
    case ScanDataType::ANY_INTEGER: return makeAnyIntegerScanRoutine(match, reverse_endianness);
    case ScanDataType::ANY_FLOAT: return makeAnyFloatScanRoutine(match, reverse_endianness);
    case ScanDataType::ANY_NUMBER: return makeAnyNumberScanRoutine(match, reverse_endianness);
    default: return nullRoutine();
  }
}
auto isRoutineAvailable(const ScanDataType type, const ScanMatchType) -> bool { return type != ScanDataType{}; }
}  // namespace scan
