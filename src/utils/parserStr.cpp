#include "newscanmem/utils/parserStr.hpp"

namespace utils {
auto isFloatToken(const std::string_view str) -> bool {
    if (str.empty()) return false;
    for (const char character : str)
        if (character == '.' || character == 'e' || character == 'E')
            return true;
    const auto lowered = StringUtils::toLower(str);
    return lowered == "nan" || lowered == "+nan" || lowered == "-nan" ||
           lowered == "inf" || lowered == "+inf" || lowered == "-inf" ||
           lowered == "infinity" || lowered == "+infinity" ||
           lowered == "-infinity";
}
auto parseDouble(const std::string_view str) -> std::optional<double> {
    if (str.empty()) return std::nullopt;
    double value = 0.0;
    const auto* end = str.data() + str.size();
    const auto [pointer, error] = std::from_chars(str.data(), end, value);
    return error == std::errc{} && pointer == end ? std::optional<double>{value}
                                                  : std::nullopt;
}
auto parseAddress(std::string_view str) -> std::optional<std::uintptr_t> {
    if (str.starts_with("0x") || str.starts_with("0X")) str.remove_prefix(2);
    if (str.empty()) return std::nullopt;
    std::uintptr_t address = 0;
    const auto [pointer, error] =
        std::from_chars(str.data(), str.data() + str.size(), address, 16);
    return error == std::errc{} && pointer == str.data() + str.size()
               ? std::optional<std::uintptr_t>{address}
               : std::nullopt;
}
auto parsePid(const std::string_view str) -> std::optional<pid_t> {
    const auto result = parseInteger<int64_t>(str);
    if (!result || *result <= 0 || *result > std::numeric_limits<pid_t>::max())
        return std::nullopt;
    return static_cast<pid_t>(*result);
}
auto parseBoolean(const std::string_view str) -> std::optional<bool> {
    const auto lowered = StringUtils::toLower(str);
    if (lowered == "true" || lowered == "yes" || lowered == "1" ||
        lowered == "on")
        return true;
    if (lowered == "false" || lowered == "no" || lowered == "0" ||
        lowered == "off")
        return false;
    return std::nullopt;
}
}  // namespace utils
