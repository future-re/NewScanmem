module;

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>

export module value.scalar;

import value.view;
import value.buffer;

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
    MemView view;

    [[nodiscard]] constexpr auto size() const noexcept -> size_t {
        return sizeOf(kind);
    }

    template <typename T>
    static auto make(const T& val, ByteBuffer& buffer) noexcept -> ScalarValue {
        static_assert(std::is_trivially_copyable_v<T>);
        const size_t START = buffer.appendValue<T>(val);
        return ScalarValue{KindOf<T>::VALUE,
                           MemView{buffer.ptr() + START, sizeof(T)}};
    }

    template <typename T>
    [[nodiscard]] auto get() const noexcept -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        if (KindOf<T>::VALUE != kind || view.size() < sizeof(T)) {
            return std::nullopt;
        }
        return view.tryGet<T>();
    }

    static auto fromBytes(ScalarKind kVal, MemView memView) noexcept
        -> std::optional<ScalarValue> {
        const auto N_VAL = sizeOf(kVal);
        if (memView.size() < N_VAL) {
            return std::nullopt;
        }
        if (auto subview = memView.subview(0, N_VAL)) {
            return ScalarValue{.kind = kVal, .view = *subview};
        }
        return std::nullopt;
    }

    [[nodiscard]] auto asVariant() const noexcept -> ScalarVariant {
        switch (kind) {
            case ScalarKind::U8:
                return *view.tryGet<uint8_t>();
            case ScalarKind::S8:
                return *view.tryGet<int8_t>();
            case ScalarKind::U16:
                return *view.tryGet<uint16_t>();
            case ScalarKind::S16:
                return *view.tryGet<int16_t>();
            case ScalarKind::U32:
                return *view.tryGet<uint32_t>();
            case ScalarKind::S32:
                return *view.tryGet<int32_t>();
            case ScalarKind::U64:
                return *view.tryGet<uint64_t>();
            case ScalarKind::S64:
                return *view.tryGet<int64_t>();
            case ScalarKind::F32:
                return *view.tryGet<float>();
            case ScalarKind::F64:
                return *view.tryGet<double>();
        }
        return ScalarVariant{};
    }
};
