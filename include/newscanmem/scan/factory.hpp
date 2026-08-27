#pragma once

/**
 * @file factory.hpp
 * @brief Canonical scan routine factory plus thin legacy wrapper
 */

#include <cstdint>

#include "newscanmem/scan/bytes.hpp"
#include "newscanmem/scan/numeric.hpp"
#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/string.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"

namespace scan {

/**
 * @brief Create a scan routine for the given data type and match type
 * @param dataType The data type to scan for
 * @param matchType The match condition
 * @param requiredFlag The required type flag (usually from user value)
 * @param reverseEndianness Whether to reverse byte order
 * @return A ScanRoutine that performs the matching
 */
[[nodiscard]] auto makeScanRoutine(ScanDataType data_type,
                                   ScanMatchType match_type,
                                   bool reverse_endianness) -> ScanRoutine;

/**
 * @brief Check if a scan routine is available for the given configuration
 */
[[nodiscard]] auto isRoutineAvailable(ScanDataType data_type,
                                      ScanMatchType match_type) -> bool;

}  // namespace scan
