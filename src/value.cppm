module;
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

export module value;

export enum class MatchFlags : uint16_t {
    Empty = 0,

    U8b = 1 << 0,   // Unsigned 8-bit
    S8b = 1 << 1,   // Signed 8-bit
    U16b = 1 << 2,  // Unsigned 16-bit
    S16b = 1 << 3,  // Signed 16-bit
    U32b = 1 << 4,  // Unsigned 32-bit
    S32b = 1 << 5,  // Signed 32-bit
    U64b = 1 << 6,  // Unsigned 64-bit
    S64b = 1 << 7,  // Signed 64-bit

    F32b = 1 << 8,  // 32-bit float
    F64b = 1 << 9,  // 64-bit float

    I8b = U8b | S8b,
    I16b = U16b | S16b,
    I32b = U32b | S32b,
    I64b = U64b | S64b,

    Integer = I8b | I16b | I32b | I64b,
    Float = F32b | F64b,
    All = Integer | Float,

    B8 = I8b,
    B16 = I16b,
    B32 = I32b | F32b,
    B64 = I64b | F64b,

    Max = 0xffffU
} __attribute__((packed));

export struct Value {
    union {
        int8_t int8_value;
        uint8_t uint8_value;
        int16_t int16_value;
        uint16_t uint16_value;
        int32_t int32_value;
        uint32_t uint32_value;
        int64_t int64_value;
        uint64_t uint64_value;
        float float32_value;
        double float64_value;
        std::array<uint8_t, sizeof(int64_t)> bytes;
        std::array<char, sizeof(int64_t)> chars;
    };

    MatchFlags flags = MatchFlags::Empty;

    // constexpr 静态函数，支持编译期调用
    constexpr static void zero(Value& val) {
        val.int64_value = 0;
        val.flags = MatchFlags::Empty;
    }
} __attribute__((packed));

struct Mem64 {
    union {
        int8_t int8_value;
        uint8_t uint8_value;
        int16_t int16_value;
        uint16_t uint16_value;
        int32_t int32_value;
        uint32_t uint32_value;
        int64_t int64_value;
        uint64_t uint64_value;
        float float32_value;
        double float64_value;
        std::array<std::byte, sizeof(int64_t)> bytes;
        std::array<char, sizeof(int64_t)> chars;
    };

    // 提供安全的访问器函数
    template <typename T>
    T get() const {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Type must be trivially copyable");
        return std::bit_cast<T>(int64_value);  // 使用 std::bit_cast 替代 memcpy
    }

    template <typename T>
    void set(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Type must be trivially copyable");
        int64_value =
            std::bit_cast<int64_t>(value);  // 使用 std::bit_cast 替代 memcpy
    }
} __attribute__((packed));

enum class Wildcard : uint8_t { FIXED = 0xffU, WILDCARD = 0x00U };

struct UserValue {
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

    const uint8_t* bytearray_value =
        nullptr;  // 可选：改为 std::optional<std::vector<uint8_t>>
    const Wildcard* wildcard_value =
        nullptr;  // 可选：改为 std::optional<Wildcard>

    const char* string_value =
        nullptr;  // 可选：改为 std::string_view 或 std::string

    MatchFlags flags = MatchFlags::Empty;  // 使用现代枚举类型
} __attribute__((packed));
