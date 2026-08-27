#include "newscanmem/scan/numeric.hpp"

namespace detail {

template auto tryNumericSequence<uint64_t, int64_t, uint32_t, int32_t, uint16_t,
                                 int16_t, uint8_t, int8_t>(
    ScanMatchType, const scan::ScanContext&,
    MatchFlags*) noexcept -> unsigned int;

template auto tryNumericSequence<double, float>(
    ScanMatchType, const scan::ScanContext&,
    MatchFlags*) noexcept -> unsigned int;

}  // namespace detail

auto makeAnyIntegerScanRoutine(ScanMatchType matchType,
                               bool reverseEndianness) -> scan::ScanRoutine {
    return [matchType, reverseEndianness](const scan::ScanContext& baseCtx) {
        scan::ScanContext ctx = baseCtx;
        ctx.reverseEndianness = reverseEndianness;
        MatchFlags flags = MatchFlags::EMPTY;
        const auto matched =
            detail::tryNumericSequence<uint64_t, int64_t, uint32_t, int32_t,
                                       uint16_t, int16_t, uint8_t, int8_t>(
                matchType, ctx, &flags);
        if (matched == 0U) {
            return scan::ScanResult::noMatch();
        }
        return scan::ScanResult::match(matched, flags);
    };
}

auto makeAnyFloatScanRoutine(ScanMatchType matchType,
                             bool reverseEndianness) -> scan::ScanRoutine {
    return [matchType, reverseEndianness](const scan::ScanContext& baseCtx) {
        scan::ScanContext ctx = baseCtx;
        ctx.reverseEndianness = reverseEndianness;
        MatchFlags flags = MatchFlags::EMPTY;
        const auto matched =
            detail::tryNumericSequence<double, float>(matchType, ctx, &flags);
        if (matched == 0U) {
            return scan::ScanResult::noMatch();
        }
        return scan::ScanResult::match(matched, flags);
    };
}

auto makeAnyNumberScanRoutine(ScanMatchType matchType,
                              bool reverseEndianness) -> scan::ScanRoutine {
    return [matchType, reverseEndianness](const scan::ScanContext& baseCtx) {
        scan::ScanContext ctx = baseCtx;
        ctx.reverseEndianness = reverseEndianness;
        MatchFlags flags = MatchFlags::EMPTY;
        if (auto matched = detail::tryNumericSequence<double, float>(
                matchType, ctx, &flags);
            matched != 0U) {
            return scan::ScanResult::match(matched, flags);
        }
        const auto matched =
            detail::tryNumericSequence<uint64_t, int64_t, uint32_t, int32_t,
                                       uint16_t, int16_t, uint8_t, int8_t>(
                matchType, ctx, &flags);
        if (matched == 0U) {
            return scan::ScanResult::noMatch();
        }
        return scan::ScanResult::match(matched, flags);
    };
}
