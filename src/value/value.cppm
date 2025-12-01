module;

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

export module value;

export import value.scalar;
export import value.flags;

// Value - byte data used to store snapshots of old values
export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes; 

    Value() = default;

    // Set byte content
    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }

    // Get byte view
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }
};

// Helper function - must be declared before UserValue
export template <typename T>
constexpr auto flagForScalarType() -> MatchFlags {
    if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
        return MatchFlags::B8;
    } else if constexpr (std::is_same_v<T, int16_t> ||
                         std::is_same_v<T, uint16_t>) {
        return MatchFlags::B16;
    } else if constexpr (std::is_same_v<T, int32_t> ||
                         std::is_same_v<T, uint32_t> ||
                         std::is_same_v<T, float>) {
        return MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, int64_t> ||
                         std::is_same_v<T, uint64_t> ||
                         std::is_same_v<T, double>) {
        return MatchFlags::B64;
    }
    return MatchFlags::EMPTY;
}

export struct UserValue {
    // Scalar values (low/high for ranges) - backward compatible
    int8_t s8 = 0, s8h = 0;
    uint8_t u8 = 0, u8h = 0;
    int16_t s16 = 0, s16h = 0;
    uint16_t u16 = 0, u16h = 0;
    int32_t s32 = 0, s32h = 0;
    uint32_t u32 = 0, u32h = 0;
    int64_t s64 = 0, s64h = 0;
    uint64_t u64 = 0, u64h = 0;
    float f32 = 0.0F, f32h = 0.0F;
    double f64 = 0.0, f64h = 0.0;

    // Complex types
    std::optional<std::vector<std::uint8_t>> bytearrayValue;
    std::optional<std::vector<std::uint8_t>>
        byteMask;  // 0xFF=fixed, 0x00=wildcard
    std::string stringValue;

    // Valid flags
    MatchFlags flags = MatchFlags::EMPTY;

    template <typename T>
    static auto fromScalar(T value) -> UserValue {
        UserValue userVal;
        setScalarValue(userVal, value);
        userVal.flags = flagForScalarType<T>();
        return userVal;
    }

    template <typename T>
    static auto fromRange(T low, T high) -> UserValue {
        UserValue userVal;
        setScalarValue(userVal, low);
        setScalarHighValue(userVal, high);
        userVal.flags = flagForScalarType<T>();
        return userVal;
    }

   private:
    template <typename T>
    static void setScalarValue(UserValue& userVal, T value) {
        if constexpr (std::is_same_v<T, int8_t>) {
            userVal.s8 = value;
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            userVal.u8 = value;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            userVal.s16 = value;
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            userVal.u16 = value;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            userVal.s32 = value;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            userVal.u32 = value;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            userVal.s64 = value;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            userVal.u64 = value;
        } else if constexpr (std::is_same_v<T, float>) {
            userVal.f32 = value;
        } else if constexpr (std::is_same_v<T, double>) {
            userVal.f64 = value;
        }
    }

    template <typename T>
    static void setScalarHighValue(UserValue& userVal, T value) {
        if constexpr (std::is_same_v<T, int8_t>) {
            userVal.s8h = value;
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            userVal.u8h = value;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            userVal.s16h = value;
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            userVal.u16h = value;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            userVal.s32h = value;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            userVal.u32h = value;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            userVal.s64h = value;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            userVal.u64h = value;
        } else if constexpr (std::is_same_v<T, float>) {
            userVal.f32h = value;
        } else if constexpr (std::is_same_v<T, double>) {
            userVal.f64h = value;
        }
    }
};
