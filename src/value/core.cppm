/**
 * @file core.cppm
 * @brief Core single-value container
 */

module;

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

export module value.core;

import value.flags;

template <typename T>
concept ValueArithmeticType = std::is_arithmetic_v<std::remove_cvref_t<T>>;

template <typename T>
concept ValueStringType = std::same_as<std::remove_cvref_t<T>, std::string>;

template <typename T>
concept ValueByteVectorType =
    std::same_as<std::remove_cvref_t<T>, std::vector<std::uint8_t>>;

export struct Value;

// ============================================================================
// Value: core single-value container
// ============================================================================
export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    // For byte array matching: 0xFF=fixed, 0x00=wildcard
    std::optional<std::vector<std::uint8_t>> mask;

   private:
    template <ValueArithmeticType T>
    [[nodiscard]] static auto makeArithmetic(T value) -> Value {
        using U = std::remove_cvref_t<T>;
        Value val;
        val.flags = flagForType<U>();
        auto raw = std::bit_cast<std::array<std::uint8_t, sizeof(U)>>(
            static_cast<U>(value));
        val.bytes.assign(raw.begin(), raw.end());
        return val;
    }

    [[nodiscard]] static auto makeString(std::string value) -> Value {
        Value val;
        val.flags = MatchFlags::STRING;
        val.bytes.assign(value.begin(), value.end());
        return val;
    }

    [[nodiscard]] static auto makeBytes(
        std::vector<std::uint8_t> data,
        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value {
        Value val;
        val.flags = MatchFlags::BYTE_ARRAY;
        val.bytes = std::move(data);
        val.mask = std::move(mask);
        if (val.mask && val.mask->size() != val.bytes.size()) {
            val.mask = std::nullopt;
        }
        return val;
    }

   public:
    Value() = default;

    explicit Value(const std::uint8_t* data, std::size_t len)
        : bytes(data, data + len) {}

    explicit Value(std::vector<std::uint8_t> data) : bytes(std::move(data)) {}

    // ====================================================================
    // Canonical factories for single values
    // ====================================================================
    template <ValueArithmeticType T>
    [[nodiscard]] static auto of(T&& value) -> Value {
        return makeArithmetic(std::forward<T>(value));
    }

    template <ValueStringType T>
    [[nodiscard]] static auto of(T&& value) -> Value {
        return makeString(std::string(std::forward<T>(value)));
    }

    template <ValueByteVectorType T>
    [[nodiscard]] static auto of(
        T&& data, std::optional<std::vector<std::uint8_t>> mask = std::nullopt)
        -> Value {
        return makeBytes(std::vector<std::uint8_t>(std::forward<T>(data)),
                         std::move(mask));
    }

    // ====================================================================
    // Canonical accessors for single values
    // ====================================================================

    template <NumericType T>
    [[nodiscard]] auto as() const -> std::optional<T> {
        if (bytes.size() != sizeof(T)) {
            return std::nullopt;
        }
        T result{};
        std::copy_n(bytes.data(), sizeof(T), (std::uint8_t*)(&result));
        return result;
    }

    [[nodiscard]] auto asString() const -> std::optional<std::string> {
        if (flags != MatchFlags::STRING) {
            return std::nullopt;
        }
        return std::string(bytes.begin(), bytes.end());
    }

    [[nodiscard]] auto asBytes() const
        -> std::optional<std::vector<std::uint8_t>> {
        if (flags != MatchFlags::BYTE_ARRAY) {
            return std::nullopt;
        }
        return bytes;
    }

    // ====================================================================
    // Basic operations
    // ====================================================================

    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }
    [[nodiscard]] auto data() noexcept -> std::uint8_t* { return bytes.data(); }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }
    [[nodiscard]] auto empty() const noexcept -> bool { return bytes.empty(); }
    [[nodiscard]] auto flag() const noexcept -> MatchFlags { return flags; }

    void clear() {
        bytes.clear();
        mask.reset();
        flags = MatchFlags::EMPTY;
    }

    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }
    void setBytes(std::vector<std::uint8_t>&& data) { bytes = std::move(data); }

    [[nodiscard]] auto hasMask() const -> bool { return mask.has_value(); }
};

// ============================================================================
// std::formatter
// ============================================================================
namespace std {

template <>
struct formatter<Value> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const Value& value, format_context& ctx) {
        if (value.flag() == MatchFlags::EMPTY) {
            return format_to(ctx.out(), "<empty>");
        }
        if (value.flag() == MatchFlags::STRING) {
            if (auto text = value.asString()) {
                return format_to(ctx.out(), "\"{}\"", *text);
            }
            return format_to(ctx.out(), "\"\"");
        }
        format_to(ctx.out(), "[");
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (i != 0U) {
                format_to(ctx.out(), " ");
            }
            format_to(ctx.out(), "{:02x}", value.data()[i]);
        }
        return format_to(ctx.out(), "]");
    }
};

}  // namespace std
