#include "newscanmem/ui/user_input.hpp"

auto UserInput::fromBytes(const std::span<const std::uint8_t> bytes)
    -> UserInput {
    UserInput input;
    input.value = UserValue::fromByteArray(
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
    input.valueType = MatchFlags::BYTE_ARRAY;
    return input;
}
auto UserInput::fromBytesWithMask(const std::span<const std::uint8_t> bytes,
                                  const std::span<const std::uint8_t> mask)
    -> UserInput {
    UserInput input;
    input.value = UserValue::fromByteArray(
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()),
        std::vector<std::uint8_t>(mask.begin(), mask.end()));
    input.valueType = MatchFlags::BYTE_ARRAY;
    return input;
}
auto UserInput::fromString(const std::string& text) -> UserInput {
    UserInput input;
    input.value = UserValue::fromString(text);
    input.valueType = MatchFlags::STRING;
    return input;
}
