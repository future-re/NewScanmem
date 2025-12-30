module;

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

export module value.flags;

// base Flags for matching types
export enum class MatchFlags : std::uint16_t {
    EMPTY = 0,
    // Width-based flags (used for marking match width/type in scan paths)
    B8 = 1U << 0,
    B16 = 1U << 1,
    B32 = 1U << 2,
    B64 = 1U << 3,
    STRING = 1U << 8,
    BYTE_ARRAY = 1U << 9,
};

// bit wise operators for MatchFlags
export constexpr auto operator|(MatchFlags aVal, MatchFlags bVal) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(static_cast<std::uint16_t>(aVal) |
                                   static_cast<std::uint16_t>(bVal));
}

export constexpr auto operator&(MatchFlags aVal, MatchFlags bVal) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(static_cast<std::uint16_t>(aVal) &
                                   static_cast<std::uint16_t>(bVal));
}

export constexpr auto operator^(MatchFlags aVal, MatchFlags bVal) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(static_cast<std::uint16_t>(aVal) ^
                                   static_cast<std::uint16_t>(bVal));
}

export constexpr auto operator~(MatchFlags aVal) noexcept -> MatchFlags {
    return static_cast<MatchFlags>(~static_cast<std::uint16_t>(aVal));
}

export inline auto operator|=(MatchFlags& aVal, MatchFlags bVal) noexcept
    -> MatchFlags& {
    aVal = (aVal | bVal);
    return aVal;
}

export inline auto operator&=(MatchFlags& aVal, MatchFlags bVal) noexcept
    -> MatchFlags& {
    aVal = (aVal & bVal);
    return aVal;
}

// check if any flag is set
export constexpr auto any(MatchFlags flag) noexcept -> bool {
    return (flag != MatchFlags::EMPTY);
}

// Null-safe flag manipulation helpers
// These functions safely handle nullptr for optional flag output parameters
export inline void setFlagsIfNotNull(MatchFlags* dest,
                                     MatchFlags flags) noexcept {
    if (dest != nullptr) {
        *dest = flags;
    }
}

export inline void orFlagsIfNotNull(MatchFlags* dest,
                                    MatchFlags flags) noexcept {
    if (dest != nullptr) {
        *dest |= flags;
    }
}

export template <typename T>
[[nodiscard]] constexpr auto flagForType() noexcept -> MatchFlags {
    if constexpr (std::is_same_v<T, std::string>) {
        return MatchFlags::STRING;
    } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
        return MatchFlags::BYTE_ARRAY;
    } else if constexpr (std::is_same_v<T, float>) {
        return MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, double>) {
        return MatchFlags::B64;
    } else if constexpr (std::is_integral_v<T>) {
        if constexpr (sizeof(T) == 1) {
            return MatchFlags::B8;
        } else if constexpr (sizeof(T) == 2) {
            return MatchFlags::B16;
        } else if constexpr (sizeof(T) == 4) {
            return MatchFlags::B32;
        } else if constexpr (sizeof(T) == 8) {
            return MatchFlags::B64;
        } else {
            return MatchFlags::EMPTY;
        }
    } else {
        return MatchFlags::EMPTY;
    }
}
