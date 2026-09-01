#pragma once

/**
 * @file session.hpp
 * @brief Shared session state for CLI commands (pid + scanner)
 */

#include <sys/types.h>

#include <memory>

#include "memseek/core/maps.hpp"
#include "memseek/core/scanner.hpp"
#include "memseek/utils/endianness.hpp"
using core::Scanner;

namespace cli {

struct SessionState {
    pid_t pid{0};
    std::unique_ptr<Scanner> scanner;
    core::RegionScanLevel regionLevel{core::RegionScanLevel::ALL_RW};
    utils::Endianness endianness{(std::endian::native == std::endian::little
                                      ? utils::Endianness::LITTLE
                                      : utils::Endianness::BIG)};

    auto ensureScanner() -> Scanner*;

    auto resetScanner() const -> void;
    auto setEndianness(utils::Endianness mode) -> void;
};

}  // namespace cli
