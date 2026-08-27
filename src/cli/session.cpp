#include "newscanmem/cli/session.hpp"

namespace cli {
auto SessionState::ensureScanner() -> Scanner* {
    if (pid <= 0) return nullptr;
    if (!scanner || scanner->getPid() != pid)
        scanner = std::make_unique<Scanner>(pid);
    return scanner.get();
}
void SessionState::resetScanner() const {
    if (scanner) scanner->reset();
}
void SessionState::setEndianness(const utils::Endianness mode) {
    endianness = mode;
}
}  // namespace cli
