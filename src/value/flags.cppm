module;

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

export module value.flags;

export template <typename T>
concept ValueArithmeticType = std::is_arithmetic_v<std::remove_cvref_t<T>>;

export template <typename T>
concept ValueStringType = std::same_as<std::remove_cvref_t<T>, std::string>;

export template <typename T>
concept ValueByteVectorType =
    std::same_as<std::remove_cvref_t<T>, std::vector<std::uint8_t>>;

export template <typename T>
concept ValueTypeConcept =
    ValueArithmeticType<T> || ValueStringType<T> || ValueByteVectorType<T>;

export enum class MatchFlags : std::uint16_t {
    EMPTY = 0,
    B8 = 1U << 0,
    B16 = 1U << 1,
    B32 = 1U << 2,
    B64 = 1U << 3,
    STRING = 1U << 8,
    BYTE_ARRAY = 1U << 9,
};

export using ValueType = MatchFlags;

namespace detail {

[[nodiscard]] constexpr auto toUnderlying(MatchFlags flags) noexcept
    -> std::underlying_type_t<MatchFlags> {
    return static_cast<std::underlying_type_t<MatchFlags>>(flags);
}

template <typename T>
[[nodiscard]] consteval auto widthFlag() noexcept -> MatchFlags {
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
}

}  // namespace detail

export [[nodiscard]] constexpr auto operator|(MatchFlags lhs,
                                              MatchFlags rhs) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(detail::toUnderlying(lhs) |
                                   detail::toUnderlying(rhs));
}

export [[nodiscard]] constexpr auto operator&(MatchFlags lhs,
                                              MatchFlags rhs) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(detail::toUnderlying(lhs) &
                                   detail::toUnderlying(rhs));
}

export [[nodiscard]] constexpr auto operator^(MatchFlags lhs,
                                              MatchFlags rhs) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(detail::toUnderlying(lhs) ^
                                   detail::toUnderlying(rhs));
}

export [[nodiscard]] constexpr auto operator~(MatchFlags flags) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(~detail::toUnderlying(flags));
}

export constexpr auto operator|=(MatchFlags& lhs, MatchFlags rhs) noexcept
    -> MatchFlags& {
    lhs = lhs | rhs;
    return lhs;
}

export constexpr auto operator&=(MatchFlags& lhs, MatchFlags rhs) noexcept
    -> MatchFlags& {
    lhs = lhs & rhs;
    return lhs;
}

export [[nodiscard]] constexpr auto any(MatchFlags flags) noexcept -> bool {
    return flags != MatchFlags::EMPTY;
}

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
[[nodiscard]] consteval auto flagForType() noexcept -> MatchFlags {
    using U = std::remove_cvref_t<T>;

    if constexpr (ValueStringType<U>) {
        return MatchFlags::STRING;
    } else if constexpr (ValueByteVectorType<U>) {
        return MatchFlags::BYTE_ARRAY;
    } else if constexpr (ValueArithmeticType<U>) {
        return detail::widthFlag<U>();
    } else {
        return MatchFlags::EMPTY;
    }
}
