// module;
// #include <array>
// #include <bit>
// #include <cstdint>
// #include <cstring>
// #include <optional>
// #include <span>
// #include <type_traits>
// #include <variant>

// export module value.base_value;

// export enum class ScalarKind : uint8_t {
//     U8,
//     S8,
//     U16,
//     S16,
//     U32,
//     S32,
//     U64,
//     S64,
//     F32,
//     F64
// };

// export using ScalarVariant =
//     std::variant<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t,
//                  uint64_t, int64_t, float, double>;

// export constexpr auto sizeOf(ScalarKind kVal) noexcept -> size_t {
//     switch (kVal) {
//         case ScalarKind::U8:
//         case ScalarKind::S8:
//             return 1;
//         case ScalarKind::U16:
//         case ScalarKind::S16:
//             return 2;
//         case ScalarKind::U32:
//         case ScalarKind::S32:
//         case ScalarKind::F32:
//             return 4;
//         case ScalarKind::U64:
//         case ScalarKind::S64:
//         case ScalarKind::F64:
//             return 8;
//     }
//     return 0;
// }

// template <typename T>
// struct KindOf;
// template <>
// struct KindOf<uint8_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::U8;
// };
// template <>
// struct KindOf<int8_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::S8;
// };
// template <>
// struct KindOf<uint16_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::U16;
// };
// template <>
// struct KindOf<int16_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::S16;
// };
// template <>
// struct KindOf<uint32_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::U32;
// };
// template <>
// struct KindOf<int32_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::S32;
// };
// template <>
// struct KindOf<uint64_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::U64;
// };
// template <>
// struct KindOf<int64_t> {
//     static constexpr ScalarKind VALUE = ScalarKind::S64;
// };
// template <>
// struct KindOf<float> {
//     static constexpr ScalarKind VALUE = ScalarKind::F32;
// };
// template <>
// struct KindOf<double> {
//     static constexpr ScalarKind VALUE = ScalarKind::F64;
// };

// export struct ScalarValue {
//     ScalarKind kind{};
//     std::array<uint8_t, 8> data{};  // 只用前 sizeOf(kind) 字节

//     [[nodiscard]] constexpr auto size() const noexcept -> size_t {
//         return sizeOf(kind);
//     }
//     [[nodiscard]] auto view() const noexcept -> std::span<const uint8_t> {
//         return {data.data(), size()};
//     }

//     template <typename T>
//     static auto make(const T& val) noexcept -> ScalarValue {
//         static_assert(std::is_trivially_copyable_v<T>);
//         ScalarValue sval{KindOf<T>::value, {}};
//         std::memcpy(sval.data.data(), &val, sizeof(T));
//         return sval;
//     }

//     template <typename T>
//     [[nodiscard]] auto get() const noexcept -> std::optional<T> {
//         static_assert(std::is_trivially_copyable_v<T>);
//         if (KindOf<T>::value != kind) {
//             return std::nullopt;
//         }
//         T out{};
//         std::memcpy(&out, data.data(), sizeof(T));
//         return out;
//     }

//     static auto fromBytes(ScalarKind kVal,
//                           std::span<const uint8_t> bytes) noexcept
//         -> std::optional<ScalarValue> {
//         const auto N_VAL = sizeOf(kVal);
//         if (bytes.size() < N_VAL) {
//             return std::nullopt;
//         }
//         ScalarValue sVal{.kind = kVal, .data = {}};
//         std::memcpy(sVal.data.data(), bytes.data(), N_VAL);
//         return sVal;
//     }

//     [[nodiscard]] auto asVariant() const noexcept -> ScalarVariant {
//         switch (kind) {
//             case ScalarKind::U8:
//                 return *std::bit_cast<const uint8_t*>(data.data());
//             case ScalarKind::S8:
//                 return *std::bit_cast<const int8_t*>(data.data());
//             case ScalarKind::U16:
//                 return *std::bit_cast<const uint16_t*>(data.data());
//             case ScalarKind::S16:
//                 return *std::bit_cast<const int16_t*>(data.data());
//             case ScalarKind::U32:
//                 return *std::bit_cast<const uint32_t*>(data.data());
//             case ScalarKind::S32:
//                 return *std::bit_cast<const int32_t*>(data.data());
//             case ScalarKind::U64:
//                 return *std::bit_cast<const uint64_t*>(data.data());
//             case ScalarKind::S64:
//                 return *std::bit_cast<const int64_t*>(data.data());
//             case ScalarKind::F32:
//                 return *std::bit_cast<const float*>(data.data());
//             case ScalarKind::F64:
//                 return *std::bit_cast<const double*>(data.data());
//         }
//         // 不会到达
//         return ScalarVariant{};
//     }
// };