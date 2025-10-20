module;

#include <cstdint>

export module value.flags;

// 基础匹配/类型标志（可按需扩展）。
// 仅提供与当前 targetmem/scan 使用的最小子集。
export enum class MatchFlags : std::uint16_t {
    EMPTY = 0,

    // 基于宽度的标志（供扫描路径标记匹配宽度/类型时使用）
    B8 = 1U << 0,
    B16 = 1U << 1,
    B32 = 1U << 2,
    B64 = 1U << 3,
};

// 按位运算符
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

// 便捷：是否为非空标志
export constexpr auto any(MatchFlags flag) noexcept -> bool {
    return (flag != MatchFlags::EMPTY);
}
