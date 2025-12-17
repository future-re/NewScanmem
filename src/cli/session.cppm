/**
 * @file session.cppm
 * @brief Shared session state for CLI commands (pid + scanner)
 */

module;

#include <sys/types.h>

#include <memory>

export module cli.session;

import core.maps;
import core.scanner;
import utils.endianness;
using core::Scanner;

export namespace cli {

struct SessionState {
    pid_t pid{0};
    std::unique_ptr<Scanner> scanner;
    core::RegionScanLevel regionLevel{core::RegionScanLevel::ALL_RW};
    utils::Endianness endianness{(std::endian::native == std::endian::little
                                      ? utils::Endianness::LITTLE
                                      : utils::Endianness::BIG)};

    auto ensureScanner() -> Scanner* {
        if (pid <= 0) {
            return nullptr;
        }
        if (!scanner) {
            scanner = std::make_unique<Scanner>(pid);
        }
        return scanner.get();
    }

    auto resetScanner() const -> void {
        if (scanner) {
            scanner->reset();
        }
    }
    auto setEndianness(utils::Endianness mode) -> void {
        endianness = mode;
        // 端序修改后，应重新建立扫描基线，由上层命令触发
    }
};

}  // namespace cli
