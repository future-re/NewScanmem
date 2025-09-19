module;
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
export module value;

export enum class MatchFlags : uint16_t {
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

// 用于存储的旧值
export struct Value {
    // 底层以字节流存储（兼容旧设计）：所有按标量写入/读取均通过字节拷贝实现
    std::vector<uint8_t> bytes;
    MatchFlags flags = MatchFlags::EMPTY;

    // 复位为零状态（清空字节并清除 flags）
    constexpr static void zero(Value& val) {
        val.bytes.clear();
        val.flags = MatchFlags::EMPTY;
    }

    // 获取只读字节视图（供 scanroutines 使用）
    [[nodiscard]] auto view() const noexcept -> std::span<const uint8_t> {
        return std::span<const uint8_t>{bytes};
    }

    // 获取可写字节视图（例如端序调整时就地修改）
    auto mutableView() noexcept -> std::span<uint8_t> {
        return {bytes.data(), bytes.size()};
    }

    // 设置字节数据（复制）
    void setBytes(std::span<const uint8_t> data) {
        bytes.assign(data.begin(), data.end());
    }
    void setBytes(const uint8_t* data, std::size_t len) {
        setBytes(std::span<const uint8_t>(data, len));
    }
    void setBytes(const std::vector<uint8_t>& dataVec) {
        setBytes(std::span<const uint8_t>(dataVec));
    }

    void setBytesWithFlag(const uint8_t* data, std::size_t len,
                          MatchFlags flagsParam) {
        setBytes(data, len);
        flags = flagsParam;
    }
    void setBytesWithFlag(const std::vector<uint8_t>& dataVec,
                          MatchFlags flagsParam) {
        setBytes(dataVec);
        flags = flagsParam;
    }

    // 按标量设置（按字节拷贝），不设置 flag
    template <typename T>
    void setScalar(const T& val) {
        static_assert(std::is_trivially_copyable_v<T>);
        auto raw = std::bit_cast<std::array<uint8_t, sizeof(T)>>(val);
        setBytes(std::span<const uint8_t>(raw.data(), raw.size()));
    }

    // 按标量设置并附带 flag
    template <typename T>
    void setScalarWithFlag(const T& val, MatchFlags flagsParam) {
        setScalar<T>(val);
        flags = flagsParam;
    }

    // 按类型自动设置正确的 flag（Typed 语义）
    template <typename T>
    void setScalarTyped(const T& val) {
        setScalar<T>(val);
        flags = deduceFlagForScalar<T>();
    }

   private:
    template <typename T>
    static constexpr auto deduceFlagForScalar() noexcept -> MatchFlags {
        if constexpr (std::is_same_v<T, uint8_t>) {
            return MatchFlags::U8B;
        } else if constexpr (std::is_same_v<T, int8_t>) {
            return MatchFlags::S8B;
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return MatchFlags::U16B;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return MatchFlags::S16B;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return MatchFlags::U32B;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return MatchFlags::S32B;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return MatchFlags::U64B;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return MatchFlags::S64B;
        } else if constexpr (std::is_same_v<T, float>) {
            return MatchFlags::F32B;
        } else if constexpr (std::is_same_v<T, double>) {
            return MatchFlags::F64B;
        } else {
            return MatchFlags::EMPTY;
        }
    }
};

// 用于当前内存的扫描值
export struct Mem64 {
    // 当前读取到的字节缓冲（通常来自进程内存读取）
    std::vector<uint8_t> buffer;

    // 读取为指定标量类型（通过 memcpy 解码），若长度不足抛出
    // std::bad_variant_access
    template <typename T>
    auto get() const -> T {
        auto opt = tryGet<T>();
        if (!opt) {
            throw std::bad_variant_access();
        }
        return *opt;
    }

    template <typename T>
    [[nodiscard]] auto tryGet() const noexcept -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        if (buffer.size() < sizeof(T)) {
            return std::nullopt;
        }
        T out{};
        std::memcpy(&out, buffer.data(), sizeof(T));
        return out;
    }

    // 提供只读字节视图
    [[nodiscard]] auto bytes() const noexcept -> std::span<const uint8_t> {
        return std::span<const uint8_t>{buffer};
    }

    // 设置字节缓冲
    void setBytes(std::span<const uint8_t> data) {
        buffer.assign(data.begin(), data.end());
    }
    void setBytes(const uint8_t* data, std::size_t len) {
        setBytes(std::span<const uint8_t>(data, len));
    }
    void setBytes(const std::vector<uint8_t>& dataVec) {
        setBytes(std::span<const uint8_t>(dataVec));
    }

    // 按标量写入（按字节拷贝）
    template <typename T>
    void set(const T& val) {
        static_assert(std::is_trivially_copyable_v<T>);
        auto raw = std::bit_cast<std::array<uint8_t, sizeof(T)>>(val);
        setBytes(std::span<const uint8_t>(raw.data(), raw.size()));
    }
};

enum class Wildcard : uint8_t { FIXED = 0xffU, WILDCARD = 0x00U };

// 用于用户传入的扫描值
export struct UserValue {
    int8_t int8Value = 0;
    int8_t int8RangeHighValue = 0;
    uint8_t uint8Value = 0;
    uint8_t uint8RangeHighValue = 0;
    int16_t int16Value = 0;
    int16_t int16RangeHighValue = 0;
    uint16_t uint16Value = 0;
    uint16_t uint16RangeHighValue = 0;
    int32_t int32Value = 0;
    int32_t int32RangeHighValue = 0;
    uint32_t uint32Value = 0;
    uint32_t uint32RangeHighValue = 0;
    int64_t int64Value = 0;
    int64_t int64RangeHighValue = 0;
    uint64_t uint64Value = 0;
    uint64_t uint64RangeHighValue = 0;
    float float32Value = 0.0F;
    float float32RangeHighValue = 0.0F;
    double float64Value = 0.0;
    double float64RangeHighValue = 0.0;

    std::optional<std::vector<uint8_t>> bytearrayValue;
    std::optional<std::vector<uint8_t>> byteMask;
    std::optional<Wildcard> wildcardValue;

    std::string stringValue;
    MatchFlags flags = MatchFlags::EMPTY;
};
