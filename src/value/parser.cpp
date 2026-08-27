#include "newscanmem/value/parser.hpp"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace value {

namespace {

auto getDataTypeMap() -> const auto& {
    using MapType = std::unordered_map<std::string_view, ScanDataType>;
    static const MapType MAP = [] {
        MapType mapType;
        mapType.emplace("any", ScanDataType::ANY_NUMBER);
        mapType.emplace("anynumber", ScanDataType::ANY_NUMBER);
        mapType.emplace("anyint", ScanDataType::ANY_INTEGER);
        mapType.emplace("anyinteger", ScanDataType::ANY_INTEGER);
        mapType.emplace("anyfloat", ScanDataType::ANY_FLOAT);
        mapType.emplace("int", ScanDataType::INTEGER_32);
        mapType.emplace("int8", ScanDataType::INTEGER_8);
        mapType.emplace("i8", ScanDataType::INTEGER_8);
        mapType.emplace("int16", ScanDataType::INTEGER_16);
        mapType.emplace("i16", ScanDataType::INTEGER_16);
        mapType.emplace("int32", ScanDataType::INTEGER_32);
        mapType.emplace("i32", ScanDataType::INTEGER_32);
        mapType.emplace("int64", ScanDataType::INTEGER_64);
        mapType.emplace("i64", ScanDataType::INTEGER_64);
        mapType.emplace("float", ScanDataType::FLOAT_32);
        mapType.emplace("f32", ScanDataType::FLOAT_32);
        mapType.emplace("float_32", ScanDataType::FLOAT_32);
        mapType.emplace("float32", ScanDataType::FLOAT_32);
        mapType.emplace("double", ScanDataType::FLOAT_64);
        mapType.emplace("f64", ScanDataType::FLOAT_64);
        mapType.emplace("float_64", ScanDataType::FLOAT_64);
        mapType.emplace("float64", ScanDataType::FLOAT_64);
        mapType.emplace("string", ScanDataType::STRING);
        mapType.emplace("str", ScanDataType::STRING);
        mapType.emplace("bytearray", ScanDataType::BYTE_ARRAY);
        mapType.emplace("bytes", ScanDataType::BYTE_ARRAY);
        return mapType;
    }();
    return MAP;
}

auto getMatchTypeMap() -> const auto& {
    using MapType = std::unordered_map<std::string_view, ScanMatchType>;
    static const MapType MAP = [] {
        MapType mapType;
        mapType.emplace("any", ScanMatchType::MATCH_ANY);
        mapType.emplace("eq", ScanMatchType::MATCH_EQUAL_TO);
        mapType.emplace("=", ScanMatchType::MATCH_EQUAL_TO);
        mapType.emplace("neq", ScanMatchType::MATCH_NOT_EQUAL_TO);
        mapType.emplace("!=", ScanMatchType::MATCH_NOT_EQUAL_TO);
        mapType.emplace("gt", ScanMatchType::MATCH_GREATER_THAN);
        mapType.emplace(">", ScanMatchType::MATCH_GREATER_THAN);
        mapType.emplace("lt", ScanMatchType::MATCH_LESS_THAN);
        mapType.emplace("<", ScanMatchType::MATCH_LESS_THAN);
        mapType.emplace("range", ScanMatchType::MATCH_RANGE);
        mapType.emplace("changed", ScanMatchType::MATCH_CHANGED);
        mapType.emplace("notchanged", ScanMatchType::MATCH_NOT_CHANGED);
        mapType.emplace("update", ScanMatchType::MATCH_NOT_CHANGED);
        mapType.emplace("inc", ScanMatchType::MATCH_INCREASED);
        mapType.emplace("increased", ScanMatchType::MATCH_INCREASED);
        mapType.emplace("dec", ScanMatchType::MATCH_DECREASED);
        mapType.emplace("decreased", ScanMatchType::MATCH_DECREASED);
        mapType.emplace("incby", ScanMatchType::MATCH_INCREASED_BY);
        mapType.emplace("decby", ScanMatchType::MATCH_DECREASED_BY);
        mapType.emplace("regex", ScanMatchType::MATCH_REGEX);
        mapType.emplace("re", ScanMatchType::MATCH_REGEX);
        return mapType;
    }();
    return MAP;
}

}  // namespace

namespace detail {

auto parseStringValue(const std::vector<std::string>& args,
                      size_t startIndex) -> std::optional<UserValue> {
    if (startIndex >= args.size()) {
        return std::nullopt;
    }
    return UserValue::fromString(args[startIndex]);
}

auto parseByteArrayValue(const std::vector<std::string>& args,
                         size_t startIndex) -> std::optional<UserValue> {
    if (startIndex >= args.size()) {
        return std::nullopt;
    }
    std::string byteStr = args[startIndex];

    if (byteStr.starts_with("0x") || byteStr.starts_with("0X")) {
        byteStr = byteStr.substr(2);
    }

    std::erase(byteStr, ' ');

    if (byteStr.size() % 2 != 0) {
        return std::nullopt;
    }

    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> mask;
    bytes.reserve(byteStr.size() / 2);
    mask.reserve(byteStr.size() / 2);

    auto parseNibble = [](char chStr, std::uint8_t& value,
                          std::uint8_t& maskValue) -> bool {
        if (chStr == '?' || chStr == '*') {
            value = 0;
            maskValue = 0;
            return true;
        }
        if (chStr >= '0' && chStr <= '9') {
            value = static_cast<std::uint8_t>(chStr - '0');
            maskValue = 0x0F;
            return true;
        }
        if (chStr >= 'a' && chStr <= 'f') {
            value = static_cast<std::uint8_t>(chStr - 'a' + 10);
            maskValue = 0x0F;
            return true;
        }
        if (chStr >= 'A' && chStr <= 'F') {
            value = static_cast<std::uint8_t>(chStr - 'A' + 10);
            maskValue = 0x0F;
            return true;
        }
        return false;
    };

    for (size_t i = 0; i < byteStr.size(); i += 2) {
        std::uint8_t hiVal = 0;
        std::uint8_t loVal = 0;
        std::uint8_t hiMask = 0;
        std::uint8_t loMask = 0;
        if (!parseNibble(byteStr[i], hiVal, hiMask) ||
            !parseNibble(byteStr[i + 1], loVal, loMask)) {
            return std::nullopt;
        }
        bytes.push_back(static_cast<std::uint8_t>((hiVal << 4) | loVal));
        mask.push_back(static_cast<std::uint8_t>((hiMask << 4) | loMask));
    }

    return UserValue::fromByteArray(std::move(bytes), std::move(mask));
}

auto parseAnyNumberValue(const std::vector<std::string>& args,
                         size_t startIndex) -> std::optional<UserValue> {
    if (startIndex >= args.size()) {
        return std::nullopt;
    }
    bool floatInput = utils::isFloatToken(args[startIndex]);
    if (floatInput) {
        return buildScalar<double>(args, startIndex, utils::parseDouble);
    }
    return buildScalar<int64_t>(args, startIndex, utils::parseInteger<int64_t>);
}

}  // namespace detail

auto parseDataType(const std::string_view token)
    -> std::optional<ScanDataType> {
    const auto lowered = utils::StringUtils::toLower(token);
    const auto& types = getDataTypeMap();
    const auto found = types.find(lowered);
    return found == types.end() ? std::nullopt
                                : std::optional<ScanDataType>{found->second};
}

auto parseMatchType(const std::string_view token)
    -> std::optional<ScanMatchType> {
    const auto lowered = utils::StringUtils::toLower(token);
    const auto& types = getMatchTypeMap();
    const auto found = types.find(lowered);
    return found == types.end() ? std::nullopt
                                : std::optional<ScanMatchType>{found->second};
}

auto buildUserValue(ScanDataType dataType, ScanMatchType matchType,
                    const std::vector<std::string>& args,
                    size_t startIndex) -> std::optional<UserValue> {
    if (!matchNeedsUserValue(matchType)) {
        return std::nullopt;
    }

    auto needRange = (matchType == ScanMatchType::MATCH_RANGE);
    if (needRange && (startIndex + 1 >= args.size())) {
        return std::nullopt;
    }

    if (needRange) {
        auto rangeValue = buildUserValueRange(dataType, args, startIndex);
        if (!rangeValue) {
            return std::nullopt;
        }
        return rangeValue;
    }

    switch (dataType) {
        case ScanDataType::INTEGER_8:
            return buildScalar<int8_t>(args, startIndex,
                                       utils::parseInteger<int8_t>);

        case ScanDataType::INTEGER_16:
            return buildScalar<int16_t>(args, startIndex,
                                        utils::parseInteger<int16_t>);

        case ScanDataType::INTEGER_32:
            return buildScalar<int32_t>(args, startIndex,
                                        utils::parseInteger<int32_t>);

        case ScanDataType::INTEGER_64:
        case ScanDataType::ANY_INTEGER:
            return buildScalar<int64_t>(args, startIndex,
                                        utils::parseInteger<int64_t>);

        case ScanDataType::FLOAT_32:
        case ScanDataType::FLOAT_64:
        case ScanDataType::ANY_FLOAT:
            return buildScalar<double>(args, startIndex, utils::parseDouble);

        case ScanDataType::ANY_NUMBER:
            return detail::parseAnyNumberValue(args, startIndex);

        case ScanDataType::STRING:
            return detail::parseStringValue(args, startIndex);

        case ScanDataType::BYTE_ARRAY:
            return detail::parseByteArrayValue(args, startIndex);

        default:
            return std::nullopt;
    }
}

auto buildUserValueRange(ScanDataType dataType,
                         const std::vector<std::string>& args,
                         size_t startIndex) -> std::optional<UserValue> {
    if (startIndex + 1 >= args.size()) {
        return std::nullopt;
    }

    switch (dataType) {
        case ScanDataType::INTEGER_8:
            return buildScalarRange<int8_t>(args, startIndex,
                                            utils::parseInteger<int8_t>);

        case ScanDataType::INTEGER_16:
            return buildScalarRange<int16_t>(args, startIndex,
                                             utils::parseInteger<int16_t>);

        case ScanDataType::INTEGER_32:
            return buildScalarRange<int32_t>(args, startIndex,
                                             utils::parseInteger<int32_t>);

        case ScanDataType::INTEGER_64:
        case ScanDataType::ANY_INTEGER:
            return buildScalarRange<int64_t>(args, startIndex,
                                             utils::parseInteger<int64_t>);

        case ScanDataType::FLOAT_32:
        case ScanDataType::FLOAT_64:
        case ScanDataType::ANY_FLOAT:
            return buildScalarRange<double>(args, startIndex,
                                            utils::parseDouble);

        case ScanDataType::ANY_NUMBER: {
            if (startIndex >= args.size()) {
                return std::nullopt;
            }
            bool floatInput = utils::isFloatToken(args[startIndex]);
            if (startIndex + 1 < args.size()) {
                floatInput =
                    floatInput || utils::isFloatToken(args[startIndex + 1]);
            }
            if (floatInput) {
                return buildScalarRange<double>(args, startIndex,
                                                utils::parseDouble);
            }
            return buildScalarRange<int64_t>(args, startIndex,
                                             utils::parseInteger<int64_t>);
        }

        default:
            return std::nullopt;
    }
}

}  // namespace value
