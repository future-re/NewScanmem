#include "newscanmem/value/core.hpp"

auto Value::makeString(std::string value) -> Value { Value result; result.flags = MatchFlags::STRING; result.bytes.assign(value.begin(), value.end()); return result; }
auto Value::makeBytes(std::vector<std::uint8_t> data, std::optional<std::vector<std::uint8_t>> mask) -> Value { Value result; result.flags = MatchFlags::BYTE_ARRAY; result.bytes = std::move(data); result.mask = std::move(mask); if (result.mask && result.mask->size() != result.bytes.size()) result.mask.reset(); return result; }
Value::Value(const std::uint8_t* data, const std::size_t size) : bytes(data, data + size) {}
Value::Value(std::vector<std::uint8_t> data) : bytes(std::move(data)) {}
auto Value::fromString(const char* value) -> Value { return makeString(std::string(value)); }
auto Value::asString() const -> std::optional<std::string> { return flags == MatchFlags::STRING ? std::optional<std::string>{std::string(bytes.begin(), bytes.end())} : std::nullopt; }
auto Value::asBytes() const -> std::optional<std::vector<std::uint8_t>> { return flags == MatchFlags::BYTE_ARRAY ? std::optional<std::vector<std::uint8_t>>{bytes} : std::nullopt; }
void Value::clear() { bytes.clear(); mask.reset(); flags = MatchFlags::EMPTY; }
void Value::setBytes(const std::uint8_t* data, const std::size_t size) { bytes.assign(data, data + size); }
void Value::setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }
void Value::setBytes(std::vector<std::uint8_t>&& data) { bytes = std::move(data); }
UserValue::UserValue(Value value) : primary(std::move(value)) {}
UserValue::UserValue(Value low, Value high) : primary(std::move(low)), secondary(std::move(high)) {}
auto UserValue::fromString(const char* value) -> UserValue { return UserValue{Value::fromString(value)}; }
