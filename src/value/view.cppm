module;

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

export module value.view;

import scan.types;
import utils.endianness;
import value.core;
import value.flags;
import value.parser;

export namespace value {

struct ParserView {
    [[nodiscard]] static auto dataType(std::string_view tok)
        -> std::optional<ScanDataType> {
        return parseDataType(tok);
    }

    [[nodiscard]] static auto matchType(std::string_view tok)
        -> std::optional<ScanMatchType> {
        return parseMatchType(tok);
    }

    [[nodiscard]] static auto userValue(
        ScanDataType dataType, ScanMatchType matchType,
        const std::vector<std::string>& args,
        size_t startIndex = 0) -> std::optional<UserValue> {
        return buildUserValue(dataType, matchType, args, startIndex);
    }
};

namespace view_detail {
template <typename ValueType>
struct ValueAccess;

template <>
struct ValueAccess<Value> {
    static auto getValue(Value& value) -> Value& { return value; }
    static auto getValue(const Value& value) -> const Value& { return value; }
    static void clearMask(Value& /*unused*/) {}
    static auto toUserValue(const Value& value) -> UserValue {
        UserValue userValue;
        userValue.byteValue = value;
        return userValue;
    }
};

template <>
struct ValueAccess<UserValue> {
    static auto getValue(UserValue& value) -> Value& { return value.byteValue; }
    static auto getValue(const UserValue& value) -> const Value& {
        return value.byteValue;
    }
    static void clearMask(UserValue& value) { value.byteMask.reset(); }
    static auto toUserValue(const UserValue& value) -> UserValue {
        return value;
    }
};
}  // namespace view_detail

template <typename ValueType>
class ValueViewBase {
   public:
    explicit ValueViewBase(ValueType& value) : m_value(&value) {}

    [[nodiscard]] auto flags() const -> MatchFlags { return getValue().flags; }

    template <typename T>
    [[nodiscard]] auto as(utils::Endianness endianness = utils::getHost()) const
        -> std::optional<T> {
        const auto& rawValue = getValue();
        if constexpr (std::is_same_v<T, std::string>) {
            if (rawValue.flags != MatchFlags::STRING) {
                return std::nullopt;
            }
            return std::string(rawValue.bytes.begin(), rawValue.bytes.end());
        } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
            if (rawValue.flags != MatchFlags::BYTE_ARRAY) {
                return std::nullopt;
            }
            return rawValue.bytes;
        } else {
            const auto REQUIRED = flagForType<T>();
            if ((rawValue.flags & REQUIRED) == MatchFlags::EMPTY) {
                return std::nullopt;
            }
            return unpackScalarBytes<T>(rawValue.data(), rawValue.size(),
                                        endianness);
        }
    }

    template <typename T>
    void set(T value, utils::Endianness endianness = utils::getHost()) {
        auto& rawValue = getValue();
        rawValue.flags = flagForType<T>();
        rawValue.bytes = packScalarBytes<T>(value, endianness);
        view_detail::ValueAccess<ValueType>::clearMask(*m_value);
    }

    void set(std::string_view text) {
        auto& rawValue = getValue();
        rawValue.flags = MatchFlags::STRING;
        rawValue.bytes.assign(text.begin(), text.end());
        view_detail::ValueAccess<ValueType>::clearMask(*m_value);
    }

    void set(const std::string& text) { set(std::string_view{text}); }

    void set(std::span<const std::uint8_t> bytes,
             std::optional<std::vector<std::uint8_t>> mask = std::nullopt) {
        auto& rawValue = getValue();
        rawValue.flags = MatchFlags::BYTE_ARRAY;
        rawValue.bytes.assign(bytes.begin(), bytes.end());
        view_detail::ValueAccess<ValueType>::clearMask(*m_value);
        if constexpr (std::is_same_v<ValueType, UserValue>) {
            m_value->byteMask = std::move(mask);
        }
    }

    void set(const std::vector<std::uint8_t>& bytes) {
        set(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
    }

    [[nodiscard]] auto toUserValue() const -> UserValue {
        return view_detail::ValueAccess<ValueType>::toUserValue(*m_value);
    }

   private:
    [[nodiscard]] auto getValue() const -> const Value& {
        return view_detail::ValueAccess<ValueType>::getValue(*m_value);
    }

    auto getValue() -> Value& {
        return view_detail::ValueAccess<ValueType>::getValue(*m_value);
    }

    ValueType* m_value = nullptr;
};

template <typename ValueType>
class ValueView;

template <>
class ValueView<Value> : public ValueViewBase<Value> {
   public:
    using ValueViewBase<Value>::ValueViewBase;
};

template <>
class ValueView<UserValue> : public ValueViewBase<UserValue> {
   public:
    using ValueViewBase<UserValue>::ValueViewBase;
};

}  // namespace value
