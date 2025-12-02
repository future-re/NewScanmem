/**
 * @file session.cppm
 * @brief Shared session state for CLI commands (pid + scanner)
 */

module;

#include <sys/types.h>

#include <memory>

export module cli.session;

import core.scanner;

using core::Scanner;

export namespace cli {

struct SessionState {
    pid_t pid{0};
    std::unique_ptr<Scanner> scanner;

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
};

}  // namespace cli
