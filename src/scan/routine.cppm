/**
 * @file routine.cppm
 * @brief Scan routine interface with modern, type-safe API
 *
 * Replaces the old C-style function signature with a clean,
 * object-oriented interface using ScanContext and ScanResult.
 */

module;

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

export module scan.routine;

import value.core;
import value.flags;

// ============================================================================
// Backward compatibility: Old scan routine type
// ============================================================================

export using scanRoutine = std::function<unsigned int(
    const Value* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;

// ============================================================================
// Modern API
// ============================================================================

export namespace scan {

/**
 * @struct ScanContext
 * @brief Context for a single scan operation at a memory location
 */
struct ScanContext {
    std::span<const std::uint8_t> memory;        ///< Current memory bytes
    std::optional<Value> oldValue;               ///< Previous value (for delta matches)
    std::optional<UserValue> userValue;          ///< User target value (for comparison)
    MatchFlags requiredFlag{MatchFlags::EMPTY};  ///< Required type flag
    bool reverseEndianness{false};               ///< Reverse byte order flag

    [[nodiscard]] auto memorySize() const noexcept -> std::size_t {
        return memory.size();
    }

    [[nodiscard]] auto hasUserValue() const noexcept -> bool {
        return userValue.has_value();
    }

    [[nodiscard]] auto hasOldValue() const noexcept -> bool {
        return oldValue.has_value();
    }
};

/**
 * @struct ScanResult
 * @brief Result of a scan operation at a memory location
 */
struct ScanResult {
    bool matched{false};           ///< Whether this location matches
    std::size_t matchLength{0};    ///< Number of bytes matched
    MatchFlags matchedFlag{MatchFlags::EMPTY};  ///< Type of match

    [[nodiscard]] static auto noMatch() -> ScanResult {
        return ScanResult{false, 0, MatchFlags::EMPTY};
    }

    [[nodiscard]] static auto match(std::size_t length, MatchFlags flag) -> ScanResult {
        return ScanResult{true, length, flag};
    }

    explicit operator bool() const noexcept { return matched; }
};

/**
 * @brief Modern scan routine type
 */
using ScanRoutine = std::function<ScanResult(const ScanContext& ctx)>;

/**
 * @brief Null scan routine that never matches
 */
[[nodiscard]] inline auto nullRoutine() -> ScanRoutine {
    return [](const ScanContext&) -> ScanResult { return ScanResult::noMatch(); };
}

/**
 * @brief Adapter from old scanRoutine to new ScanRoutine
 * @param oldRoutine The old-style routine
 * @return A new-style ScanRoutine wrapper
 */
[[nodiscard]] inline auto adaptRoutine(scanRoutine oldRoutine) -> ScanRoutine {
    if (!oldRoutine) {
        return nullRoutine();
    }
    return [oldRoutine](const ScanContext& ctx) -> ScanResult {
        // Convert span to Value for old API
        Value memValue;
        memValue.bytes.assign(ctx.memory.begin(), ctx.memory.end());
        
        MatchFlags flags = MatchFlags::EMPTY;
        unsigned int len = oldRoutine(
            &memValue,
            ctx.memory.size(),
            ctx.oldValue ? &*ctx.oldValue : nullptr,
            ctx.userValue ? &*ctx.userValue : nullptr,
            &flags
        );
        
        if (len > 0) {
            return ScanResult::match(len, flags);
        }
        return ScanResult::noMatch();
    };
}

}  // namespace scan
