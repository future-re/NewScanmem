#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

template <typename T>
concept ValueArithmeticType = std::is_arithmetic_v<std::remove_cvref_t<T>>;

template <typename T>
concept ValueStringType = std::same_as<std::remove_cvref_t<T>, std::string>;

template <typename T>
concept ValueByteVectorType =
    std::same_as<std::remove_cvref_t<T>, std::vector<std::uint8_t>>;

template <typename T>
concept ValueTypeConcept =
    ValueArithmeticType<T> || ValueStringType<T> || ValueByteVectorType<T>;

enum class MatchFlags : std::uint16_t {
    EMPTY = 0,
    B8 = 1U << 0,
    B16 = 1U << 1,
    B32 = 1U << 2,
    B64 = 1U << 3,
    STRING = 1U << 8,
    BYTE_ARRAY = 1U << 9,
};

using ValueType = MatchFlags;

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

[[nodiscard]] constexpr auto operator|(MatchFlags lhs,
                                       MatchFlags rhs) noexcept -> MatchFlags {
    return static_cast<MatchFlags>(detail::toUnderlying(lhs) |
                                   detail::toUnderlying(rhs));
}

[[nodiscard]] constexpr auto operator&(MatchFlags lhs,
                                       MatchFlags rhs) noexcept -> MatchFlags {
    return static_cast<MatchFlags>(detail::toUnderlying(lhs) &
                                   detail::toUnderlying(rhs));
}

[[nodiscard]] constexpr auto operator^(MatchFlags lhs,
                                       MatchFlags rhs) noexcept -> MatchFlags {
    return static_cast<MatchFlags>(detail::toUnderlying(lhs) ^
                                   detail::toUnderlying(rhs));
}

[[nodiscard]] constexpr auto operator~(MatchFlags flags) noexcept
    -> MatchFlags {
    return static_cast<MatchFlags>(~detail::toUnderlying(flags));
}

constexpr auto operator|=(MatchFlags& lhs,
                          MatchFlags rhs) noexcept -> MatchFlags& {
    lhs = lhs | rhs;
    return lhs;
}

constexpr auto operator&=(MatchFlags& lhs,
                          MatchFlags rhs) noexcept -> MatchFlags& {
    lhs = lhs & rhs;
    return lhs;
}

[[nodiscard]] constexpr auto any(MatchFlags flags) noexcept -> bool {
    return flags != MatchFlags::EMPTY;
}

void setFlagsIfNotNull(MatchFlags* dest, MatchFlags flags) noexcept;

void orFlagsIfNotNull(MatchFlags* dest, MatchFlags flags) noexcept;

template <typename T>
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
