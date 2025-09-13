module;
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
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

export constexpr auto operator&(MatchFlags lhs, MatchFlags rhs) -> MatchFlags {
    using T = std::underlying_type_t<MatchFlags>;
    return static_cast<MatchFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

export constexpr auto operator|(MatchFlags lhs, MatchFlags rhs) -> MatchFlags {
    using T = std::underlying_type_t<MatchFlags>;
    return static_cast<MatchFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

export constexpr auto operator~(MatchFlags flag) -> MatchFlags {
    using T = std::underlying_type_t<MatchFlags>;
    return static_cast<MatchFlags>(~static_cast<T>(flag));
}

// ---- 数值类型 -> Flag 的编译期映射与 Flag 辅助 ----

template <typename T>
struct FlagOfType;

template <>
struct FlagOfType<int8_t> {
    static constexpr MatchFlags VALUE = MatchFlags::S8B;
};
template <>
struct FlagOfType<uint8_t> {
    static constexpr MatchFlags VALUE = MatchFlags::U8B;
};
template <>
struct FlagOfType<int16_t> {
    static constexpr MatchFlags VALUE = MatchFlags::S16B;
};
template <>
struct FlagOfType<uint16_t> {
    static constexpr MatchFlags VALUE = MatchFlags::U16B;
};
template <>
struct FlagOfType<int32_t> {
    static constexpr MatchFlags VALUE = MatchFlags::S32B;
};
template <>
struct FlagOfType<uint32_t> {
    static constexpr MatchFlags VALUE = MatchFlags::U32B;
};
template <>
struct FlagOfType<int64_t> {
    static constexpr MatchFlags VALUE = MatchFlags::S64B;
};
template <>
struct FlagOfType<uint64_t> {
    static constexpr MatchFlags VALUE = MatchFlags::U64B;
};
template <>
struct FlagOfType<float> {
    static constexpr MatchFlags VALUE = MatchFlags::F32B;
};
template <>
struct FlagOfType<double> {
    static constexpr MatchFlags VALUE = MatchFlags::F64B;
};

template <typename T>
constexpr auto flagOfType() -> MatchFlags {
    return FlagOfType<T>::VALUE;
}

export inline auto isNumericFlag(MatchFlags flag) -> bool {
    return (flag & MatchFlags::ALL) != MatchFlags::EMPTY;
}

export inline auto isFloatFlag(MatchFlags flag) -> bool {
    return (flag & MatchFlags::FLOAT) != MatchFlags::EMPTY;
}

export inline auto widthFromFlags(MatchFlags flag)
    -> std::optional<std::size_t> {
    if ((flag & (MatchFlags::F64B | MatchFlags::S64B | MatchFlags::U64B)) !=
        MatchFlags::EMPTY) {
        return 8;
    }
    if ((flag & (MatchFlags::F32B | MatchFlags::S32B | MatchFlags::U32B)) !=
        MatchFlags::EMPTY) {
        return 4;
    }
    if ((flag & (MatchFlags::S16B | MatchFlags::U16B)) != MatchFlags::EMPTY) {
        return 2;
    }
    if ((flag & (MatchFlags::S8B | MatchFlags::U8B)) != MatchFlags::EMPTY) {
        return 1;
    }
    return std::nullopt;
}

export struct [[gnu::packed]] Value {
    // 历史值统一以字节形式保存
    std::vector<uint8_t> bytes;
    MatchFlags flags = MatchFlags::EMPTY;

    // constexpr 静态函数，支持编译期调用
    constexpr static void zero(Value& val) {
        // 注意：constexpr 下不允许调用非 constexpr 的分配；此处仅清零语义
        val.bytes.clear();
        val.flags = MatchFlags::EMPTY;
    }

    // 访问字节视图
    [[nodiscard]] auto view() const noexcept -> std::span<const uint8_t> {
        return {bytes.data(), bytes.size()};
    }

    // 设置为任意字节序列
    void setBytes(const uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }
    void setBytes(const std::vector<uint8_t>& val) { bytes = val; }
    void setBytesWithFlag(const uint8_t* data, std::size_t len,
                          MatchFlags flag) {
        setBytes(data, len);
        flags = flag;
    }
    void setBytesWithFlag(const std::vector<uint8_t>& val, MatchFlags flag) {
        setBytes(val);
        flags = flag;
    }

    // 设置为标量类型（按其内存表示拷贝字节）
    template <typename T>
    void setScalar(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable");
        bytes.resize(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
    }
    template <typename T>
    void setScalarWithFlag(const T& value, MatchFlags flag) {
        setScalar(value);
        this->flags = flag;
    }
    template <typename T>
    void setScalarTyped(const T& value) {
        setScalar(value);
        this->flags = flagOfType<T>();
    }
};

export struct [[gnu::packed]] Mem64 {
    // 统一为纯字节缓冲区存储
    std::vector<uint8_t> buffer;

    // 读取为标量类型 T（从开头 memcpy 指定字节数）
    template <typename T>
    auto get() const -> T {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Type must be trivially copyable");
        if (buffer.size() < sizeof(T)) {
            throw std::bad_variant_access();
        }
        T out{};
        std::memcpy(&out, buffer.data(), sizeof(T));
        return out;
    }

    // 返回只读字节视图
    [[nodiscard]] auto bytes() const noexcept -> std::span<const uint8_t> {
        return {buffer.data(), buffer.size()};
    }

    // 设置原始字节
    void setBytes(const uint8_t* data, std::size_t len) {
        buffer.assign(data, data + len);
    }

    void setBytes(const std::vector<uint8_t>& data) { buffer = data; }

    // 从字符串设置（按字节复制，允许包含'\0'）
    void setString(const std::string& sval) {
        buffer.assign(sval.data(), sval.data() + sval.size());
    }

    // 设置标量（拷贝其字节表述）
    template <typename T>
    void setScalar(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Type must be trivially copyable");
        buffer.resize(sizeof(T));
        std::memcpy(buffer.data(), &value, sizeof(T));
    }
};

enum class Wildcard : uint8_t { FIXED = 0xffU, WILDCARD = 0x00U };

export struct [[gnu::packed]] UserValue {
    int8_t int8Value = 0;
    uint8_t uint8Value = 0;
    int16_t int16Value = 0;
    uint16_t uint16Value = 0;
    int32_t int32Value = 0;
    uint32_t uint32Value = 0;
    int64_t int64Value = 0;
    uint64_t uint64Value = 0;
    float float32Value = 0.0F;
    double float64Value = 0.0;

    // Range 上界（与对应类型字段一起使用：低端 = *Value，高端 =
    // *RangeHighValue）
    int8_t int8RangeHighValue = 0;
    uint8_t uint8RangeHighValue = 0;
    int16_t int16RangeHighValue = 0;
    uint16_t uint16RangeHighValue = 0;
    int32_t int32RangeHighValue = 0;
    uint32_t uint32RangeHighValue = 0;
    int64_t int64RangeHighValue = 0;
    uint64_t uint64RangeHighValue = 0;
    float float32RangeHighValue = 0.0F;
    double float64RangeHighValue = 0.0;

    std::optional<std::vector<uint8_t>> bytearrayValue;
    // 与 bytearray/string 同长的掩码：0xFF=该字节需精确匹配；0x00=通配
    std::optional<std::vector<uint8_t>> byteMask;
    std::optional<Wildcard> wildcardValue;

    std::string stringValue;
    MatchFlags flags = MatchFlags::EMPTY;
};
