#include "memseek/scan/routine.hpp"

namespace scan {
auto nullRoutine() -> ScanRoutine {
    return [](const ScanContext&) { return ScanResult::noMatch(); };
}
auto makeScanContext(const std::span<const std::uint8_t> memory,
                     const Value* oldValue, const UserValue* userValue,
                     const MatchFlags flag,
                     const bool reverseEndianness) -> ScanContext {
    ScanContext context{.memory = memory,
                        .requiredFlag = flag,
                        .reverseEndianness = reverseEndianness};
    if (oldValue != nullptr) context.oldValue = *oldValue;
    if (userValue != nullptr) context.userValue = *userValue;
    return context;
}
}  // namespace scan
