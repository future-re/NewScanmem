module;
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
export module value;

export enum class [[gnu::packed]] MatchFlags : uint16_t {
    EMPTY = 0,

    U8B = 1 << 0,   // Unsigned 8-bit
    S8B = 1 << 1,   // Signed 8-bit
    U16B = 1 << 2,  // Unsigned 16-bit
    S16B = 1 << 3,  // Signed 16-bit
    U32B = 1 << 4,  // Unsigned 32-bit
    S32B = 1 << 5,  // Signed 32-bit
    U64B = 1 << 6,  // Unsigned 64-bit
    S64B = 1 << 7,  // Signed 64-bit

    F32B = 1 << 8,  // 32-bit float
    F64B = 1 << 9,  // 64-bit float

    I8B = U8B | S8B,
    I16B = U16B | S16B,
    I32B = U32B | S32B,
    I64B = U64B | S64B,

    INTEGER = I8B | I16B | I32B | I64B,
    FLOAT = F32B | F64B,
    ALL = INTEGER | FLOAT,

    B8 = I8B,
    B16 = I16B,
    B32 = I32B | F32B,
    B64 = I64B | F64B,

    MAX = 0xffffU
};

export constexpr MatchFlags operator&(MatchFlags lhs, MatchFlags rhs) {
    using T = std::underlying_type_t<MatchFlags>;
    return static_cast<MatchFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

export constexpr MatchFlags operator|(MatchFlags lhs, MatchFlags rhs) {
    using T = std::underlying_type_t<MatchFlags>;
    return static_cast<MatchFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

export constexpr MatchFlags operator~(MatchFlags flag) {
    using T = std::underlying_type_t<MatchFlags>;
    return static_cast<MatchFlags>(~static_cast<T>(flag));
}

export struct [[gnu::packed]] Value {
    std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, float, double, std::array<uint8_t, sizeof(int64_t)>,
                 std::array<char, sizeof(int64_t)>>
        value;

    MatchFlags flags = MatchFlags::EMPTY;

    // constexpr 静态函数，支持编译期调用
    constexpr static void zero(Value& val) {
        val.value = int64_t{0};
        val.flags = MatchFlags::EMPTY;
    }
};

struct [[gnu::packed]] Mem64 {
    std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, float, double, std::array<uint8_t, sizeof(int64_t)>,
                 std::array<char, sizeof(int64_t)>>
        mem64Value;

    // 提供安全的访问器函数
    template <typename T>
    T get() const {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Type must be trivially copyable");
        if (auto* value = std::get_if<T>(&mem64Value)) {
            return *value;
        }
        throw std::bad_variant_access();  // 如果类型不匹配，抛出异常
    }

    // 提供通用的访问接口
    template <typename Visitor>
    void visit(Visitor&& visitor) const {
        std::visit(std::forward<Visitor>(visitor), mem64Value);
    }

    template <typename T>
    void set(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Type must be trivially copyable");
        mem64Value = std::bit_cast<int64_t>(value);
    }
};

enum class Wildcard : uint8_t { FIXED = 0xffU, WILDCARD = 0x00U };

struct [[gnu::packed]] UserValue {
    int8_t int8_value = 0;
    uint8_t uint8_value = 0;
    int16_t int16_value = 0;
    uint16_t uint16_value = 0;
    int32_t int32_value = 0;
    uint32_t uint32_value = 0;
    int64_t int64_value = 0;
    uint64_t uint64_value = 0;
    float float32_value = 0.0F;
    double float64_value = 0.0;

    std::optional<std::vector<uint8_t>> bytearray_value;
    std::optional<Wildcard> wildcard_value;

    std::string string_value;
    MatchFlags flags = MatchFlags::EMPTY;
};
