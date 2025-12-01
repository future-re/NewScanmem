module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>
#include <variant>

export module value.scalar;

export enum class ScalarKind : uint8_t {
    U8,
    S8,
    U16,
    S16,
    U32,
    S32,
    U64,
    S64,
    F32,
    F64
};

export using ScalarVariant =
    std::variant<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
                 uint64_t, int64_t, float, double>;

export constexpr auto sizeOf(ScalarKind kType) noexcept -> size_t {
    switch (kType) {
        case ScalarKind::U8:
        case ScalarKind::S8:
            return 1;
        case ScalarKind::U16:
        case ScalarKind::S16:
            return 2;
        case ScalarKind::U32:
        case ScalarKind::S32:
        case ScalarKind::F32:
            return 4;
        case ScalarKind::U64:
        case ScalarKind::S64:
        case ScalarKind::F64:
            return 8;
    }
    return 0;
}

export template <typename T>
struct KindOf;
export template <>
struct KindOf<uint8_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U8;
};

export template <>
struct KindOf<int8_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S8;
};

export template <>
struct KindOf<uint16_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U16;
};

export template <>
struct KindOf<int16_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S16;
};

export template <>
struct KindOf<uint32_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U32;
};

export template <>
struct KindOf<int32_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S32;
};

export template <>
struct KindOf<uint64_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U64;
};

export template <>
struct KindOf<int64_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S64;
};

export template <>
struct KindOf<float> {
    static constexpr ScalarKind VALUE = ScalarKind::F32;
};

export template <>
struct KindOf<double> {
    static constexpr ScalarKind VALUE = ScalarKind::F64;
};

export struct ScalarValue {
    ScalarKind kind{};
    ScalarVariant value;  // store concrete scalar directly

    [[nodiscard]] constexpr auto size() const noexcept -> size_t {
        return sizeOf(kind);
    }

    // Construct from concrete scalar T (<= 8 bytes)
    template <typename T>
    static auto make(const T& val) noexcept -> ScalarValue {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= 8, "ScalarValue supports up to 8 bytes");
        ScalarValue out{KindOf<T>::VALUE};
        out.value = val;
        return out;
    }

    template <typename T>
    [[nodiscard]] auto get() const noexcept -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        if (const auto* pval = std::get_if<T>(&value)) {
            return *pval;
        }
        return std::nullopt;
    }

    static auto fromBytes(ScalarKind kVal, const uint8_t* data,
                          size_t size) noexcept -> std::optional<ScalarValue> {
        const auto N_VAL = sizeOf(kVal);
        if (size < N_VAL) {
            return std::nullopt;
        }
        ScalarValue out{.kind = kVal};
        switch (kVal) {
            case ScalarKind::U8: {
                uint8_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::S8: {
                int8_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::U16: {
                uint16_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::S16: {
                int16_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::U32: {
                uint32_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::S32: {
                int32_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::U64: {
                uint64_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::S64: {
                int64_t val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::F32: {
                float val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
            case ScalarKind::F64: {
                double val = 0;
                std::memcpy(&val, data, sizeof(val));
                out.value = val;
                return out;
            }
        }
        return std::nullopt;
    }

    template <typename T>
    static auto readFromAddress(const void* address) noexcept
        -> std::optional<ScalarValue> {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable");
        if (address == nullptr) {
            return std::nullopt;
        }
        T value;
        std::memcpy(&value, address, sizeof(T));
        return make<T>(value);
    }

    [[nodiscard]] auto asVariant() const noexcept -> ScalarVariant {
        return value;
    }
};
