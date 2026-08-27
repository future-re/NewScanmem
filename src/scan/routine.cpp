#include "newscanmem/scan/routine.hpp"

namespace scan {
auto nullRoutine() -> ScanRoutine {
    return [](const ScanContext&) { return ScanResult::noMatch(); };
}
auto makeScanContext(const std::span<const std::uint8_t> memory,
                     const Value* old_value, const UserValue* user_value,
                     const MatchFlags flag,
                     const bool reverse_endianness) -> ScanContext {
    ScanContext context{.memory = memory,
                        .requiredFlag = flag,
                        .reverseEndianness = reverse_endianness};
    if (old_value != nullptr) context.oldValue = *old_value;
    if (user_value != nullptr) context.userValue = *user_value;
    return context;
}
}  // namespace scan
