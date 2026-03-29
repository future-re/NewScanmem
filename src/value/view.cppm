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
        size_t startIndex = 0) -> std::optional<Value> {
        return buildUserValue(dataType, matchType, args, startIndex);
    }
};

class ValueView {
   public:
    explicit ValueView(Value& value) : m_value(&value) {}

    [[nodiscard]] auto flags() const -> MatchFlags { return m_value->flags; }

    template <typename T>
    [[nodiscard]] auto as(utils::Endianness endianness = utils::getHost()) const
        -> std::optional<T> {
        const auto& rawValue = *m_value;
        if constexpr (std::is_same_v<T, std::string>) {
            return rawValue.asString();
        } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
            return rawValue.asBytes();
        } else {
            return rawValue.as<T>();
        }
    }

    template <typename T>
    void set(T value, utils::Endianness endianness = utils::getHost()) {
        m_value->flags = flagForType<T>();
        m_value->bytes = packScalarBytes<T>(value, endianness);
        m_value->mask.reset();
    }

    void set(std::string_view text) {
        m_value->flags = MatchFlags::STRING;
        m_value->bytes.assign(text.begin(), text.end());
        m_value->mask.reset();
    }

    void set(const std::string& text) { set(std::string_view{text}); }

    void set(std::span<const std::uint8_t> bytes,
             std::optional<std::vector<std::uint8_t>> mask = std::nullopt) {
        m_value->flags = MatchFlags::BYTE_ARRAY;
        m_value->bytes.assign(bytes.begin(), bytes.end());
        m_value->mask = std::move(mask);
    }

    void set(const std::vector<std::uint8_t>& bytes) {
        set(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
    }

    [[nodiscard]] auto getValue() const -> const Value& { return *m_value; }
    [[nodiscard]] auto getValue() -> Value& { return *m_value; }

   private:
    Value* m_value = nullptr;
};

}  // namespace value
