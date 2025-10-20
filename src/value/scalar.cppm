module;

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>
#include <variant>

export module value.scalar;

import value.view;
import utils.endianness;

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

    static auto fromBytes(ScalarKind kVal, MemView memView) noexcept
        -> std::optional<ScalarValue> {
        const auto N_VAL = sizeOf(kVal);
        if (memView.size() < N_VAL) {
            return std::nullopt;
        }
        switch (kVal) {
            case ScalarKind::U8: {
                if (auto val = memView.tryGet<uint8_t>()) {
                    return make<uint8_t>(*val);
                }
                break;
            }
            case ScalarKind::S8: {
                if (auto val = memView.tryGet<int8_t>()) {
                    return make<int8_t>(*val);
                }
                break;
            }
            case ScalarKind::U16: {
                if (auto val = memView.tryGet<uint16_t>()) {
                    return make<uint16_t>(*val);
                }
                break;
            }
            case ScalarKind::S16: {
                if (auto val = memView.tryGet<int16_t>()) {
                    return make<int16_t>(*val);
                }
                break;
            }
            case ScalarKind::U32: {
                if (auto val = memView.tryGet<uint32_t>()) {
                    return make<uint32_t>(*val);
                }
                break;
            }
            case ScalarKind::S32: {
                if (auto val = memView.tryGet<int32_t>()) {
                    return make<int32_t>(*val);
                }
                break;
            }
            case ScalarKind::U64: {
                if (auto val = memView.tryGet<uint64_t>()) {
                    return make<uint64_t>(*val);
                }
                break;
            }
            case ScalarKind::S64: {
                if (auto val = memView.tryGet<int64_t>()) {
                    return make<int64_t>(*val);
                }
                break;
            }
            case ScalarKind::F32: {
                if (auto val = memView.tryGet<float>()) {
                    return make<float>(*val);
                }
                break;
            }
            case ScalarKind::F64: {
                if (auto val = memView.tryGet<double>()) {
                    return make<double>(*val);
                }
                break;
            }
        }
        return std::nullopt;
    }

    template <typename T>
    static auto readFromAddress(const void* address, Endian endian) noexcept
        -> std::optional<ScalarValue> {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable");
        if (address == nullptr) {
            return std::nullopt;  // 地址无效
        }
        ScalarValue sval;
        sval.kind = KindOf<T>::VALUE;
        T value;
        std::memcpy(&value, address, sizeof(T));  // 从地址读取值

        // 根据字节序转换值
        if (endian == Endian::BIG) {
            value = endianness::hostToNetwork(value);
        } else if (endian == Endian::LITTLE) {
            value = endianness::hostToLittleEndian(value);
        }

        sval.value = value;  // 将转换后的值存入 ScalarValue
        return sval;
    }

    [[nodiscard]] auto asVariant() const noexcept -> ScalarVariant {
        return value;
    }
};
