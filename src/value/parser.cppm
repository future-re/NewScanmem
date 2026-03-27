/**
 * @file parser.cppm
 * @brief Value parsing utilities for scan operations
 */
module;
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

export module value.parser;

import scan.types;
import utils.parserStr;
import utils.string;
import value.flags;
import value.core;

export namespace value {

// Forward declarations
template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalar(const std::vector<std::string>& args,
                                      size_t startIndex, Parser parser)
    -> std::optional<UserValue>;

[[nodiscard]] inline auto buildUserValueRange(
    ScanDataType dataType, const std::vector<std::string>& args,
    size_t startIndex) -> std::optional<UserValueRange>;

namespace detail {
[[nodiscard]] inline auto parseStringValue(const std::vector<std::string>& args,
                                           size_t startIndex)
    -> std::optional<UserValue> {
    if (startIndex >= args.size()) {
        return std::nullopt;
    }
    return UserValue::fromValue<std::string>(args[startIndex]);
}

[[nodiscard]] inline auto parseByteArrayValue(
    const std::vector<std::string>& args,
    size_t startIndex) -> std::optional<UserValue> {
    if (startIndex >= args.size()) {
        return std::nullopt;
    }
    std::string byteStr = args[startIndex];

    // Strip 0x/0X prefix.
    if (byteStr.starts_with("0x") || byteStr.starts_with("0X")) {
        byteStr = byteStr.substr(2);
    }

    // Remove all spaces.
    std::erase(byteStr, ' ');

    // Two chars per byte.
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

    return UserValue::fromValue<std::vector<std::uint8_t>>(std::move(bytes),
                                                           std::move(mask));
}

[[nodiscard]] inline auto parseAnyNumberValue(
    const std::vector<std::string>& args,
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

/**
 * @brief Parse data type from string with aliases support
 * @param tok Token to parse (e.g., "int", "int64", "float")
 * @return Optional ScanDataType
 */
[[nodiscard]] inline auto parseDataType(std::string_view tok)
    -> std::optional<ScanDataType> {
    const auto TO_STR = utils::StringUtils::toLower(tok);
    if (TO_STR == "any" || TO_STR == "anynumber") {
        return ScanDataType::ANY_NUMBER;
    }
    if (TO_STR == "anyint" || TO_STR == "anyinteger") {
        return ScanDataType::ANY_INTEGER;
    }
    if (TO_STR == "anyfloat") {
        return ScanDataType::ANY_FLOAT;
    }
    if (TO_STR == "int") {
        return ScanDataType::INTEGER_32;
    }
    if (TO_STR == "int8" || TO_STR == "i8") {
        return ScanDataType::INTEGER_8;
    }
    if (TO_STR == "int16" || TO_STR == "i16") {
        return ScanDataType::INTEGER_16;
    }
    if (TO_STR == "int32" || TO_STR == "i32") {
        return ScanDataType::INTEGER_32;
    }
    if (TO_STR == "int64" || TO_STR == "i64") {
        return ScanDataType::INTEGER_64;
    }
    if (TO_STR == "float" || TO_STR == "f32" || TO_STR == "float_32" ||
        TO_STR == "float32") {
        return ScanDataType::FLOAT_32;
    }
    if (TO_STR == "double" || TO_STR == "f64" || TO_STR == "float_64" ||
        TO_STR == "float64") {
        return ScanDataType::FLOAT_64;
    }
    if (TO_STR == "string" || TO_STR == "str") {
        return ScanDataType::STRING;
    }
    if (TO_STR == "bytearray" || TO_STR == "bytes") {
        return ScanDataType::BYTE_ARRAY;
    }
    return std::nullopt;
}

/**
 * @brief Parse match type from string with aliases support
 * @param tok Token to parse (e.g., "=", "eq", "changed")
 * @return Optional ScanMatchType
 */
[[nodiscard]] inline auto parseMatchType(std::string_view tok)
    -> std::optional<ScanMatchType> {
    const auto MATCH_STR = utils::StringUtils::toLower(tok);
    if (MATCH_STR == "any") {
        return ScanMatchType::MATCH_ANY;
    }
    if (MATCH_STR == "eq" || MATCH_STR == "=") {
        return ScanMatchType::MATCH_EQUAL_TO;
    }
    if (MATCH_STR == "neq" || MATCH_STR == "!=") {
        return ScanMatchType::MATCH_NOT_EQUAL_TO;
    }
    if (MATCH_STR == "gt" || MATCH_STR == ">") {
        return ScanMatchType::MATCH_GREATER_THAN;
    }
    if (MATCH_STR == "lt" || MATCH_STR == "<") {
        return ScanMatchType::MATCH_LESS_THAN;
    }
    if (MATCH_STR == "range") {
        return ScanMatchType::MATCH_RANGE;
    }
    if (MATCH_STR == "changed") {
        return ScanMatchType::MATCH_CHANGED;
    }
    if (MATCH_STR == "notchanged" || MATCH_STR == "update") {
        return ScanMatchType::MATCH_NOT_CHANGED;
    }
    if (MATCH_STR == "inc" || MATCH_STR == "increased") {
        return ScanMatchType::MATCH_INCREASED;
    }
    if (MATCH_STR == "dec" || MATCH_STR == "decreased") {
        return ScanMatchType::MATCH_DECREASED;
    }
    if (MATCH_STR == "incby") {
        return ScanMatchType::MATCH_INCREASED_BY;
    }
    if (MATCH_STR == "decby") {
        return ScanMatchType::MATCH_DECREASED_BY;
    }
    if (MATCH_STR == "regex" || MATCH_STR == "re") {
        return ScanMatchType::MATCH_REGEX;
    }
    return std::nullopt;
}

template <typename F>
constexpr auto relTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-5F);
    } else {
        return static_cast<F>(1E-12);
    }
}

template <typename F>
constexpr auto absTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-6F);
    } else {
        return static_cast<F>(1E-12);
    }
}

template <typename F>
[[nodiscard]] inline auto almostEqual(F firstValue,
                                      F secondValue) noexcept -> bool {
    using std::fabs;
    const F DIFFERENCE_VALUE = fabs(firstValue - secondValue);
    const F SCALE_VALUE =
        std::max(F(1), std::max(fabs(firstValue), fabs(secondValue)));
    return DIFFERENCE_VALUE <= std::max(absTol<F>(), relTol<F>() * SCALE_VALUE);
}

/**
 * @brief Helper: build UserValue for scalar of type T
 * @tparam T Scalar type (int8_t, int16_t, int32_t, int64_t, float, double)
 * @tparam Parser Parser function type
 * @param args Argument list
 * @param startIndex Start index in args
 * @param parser Parser function (parseInteger<T> or parseDouble)
 * @return Optional UserValue
 */
template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalar(const std::vector<std::string>& args,
                                      size_t startIndex, Parser parser)
    -> std::optional<UserValue> {
    auto valueOpt = parser(args[startIndex]);
    if (!valueOpt) {
        return std::nullopt;
    }

    return UserValue::fromValue<T>(*valueOpt);
}

/**
 * @brief Helper: build UserValue for scalar range of type T
 * @tparam T Scalar type (int8_t, int16_t, int32_t, int64_t, float, double)
 * @tparam Parser Parser function type
 * @param args Argument list
 * @param startIndex Start index in args
 * @param parser Parser function (parseInteger<T> or parseDouble)
 * @return Optional UserValueRange
 */
template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalarRange(const std::vector<std::string>& args,
                                           size_t startIndex, Parser parser)
    -> std::optional<UserValueRange> {
    auto valueLOpt = parser(args[startIndex]);
    if (!valueLOpt) {
        return std::nullopt;
    }
    auto valueROpt = parser(args[startIndex + 1]);
    if (!valueROpt) {
        return std::nullopt;
    }
    return UserValueRange{UserValue::fromValue<T>(*valueLOpt),
                          UserValue::fromValue<T>(*valueROpt)};
}

/**
 * @brief Build UserValue from parsed arguments
 * @param dataType Data type of the value
 * @param matchType Match type (affects whether range is needed)
 * @param args Argument list
 * @param startIndex Index to start parsing from
 * @return Optional UserValue
 */
[[nodiscard]] inline auto buildUserValue(
    ScanDataType dataType, ScanMatchType matchType,
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

        auto result = std::move(rangeValue->first);
        result.secondaryByteValue = std::move(rangeValue->second.byteValue);
        return result;
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

/**
 * @brief Build UserValueRange from parsed arguments for range matching
 * @param dataType Data type of the values
 * @param args Argument list
 * @param startIndex Index to start parsing from
 * @return Optional UserValueRange (pair of two UserValues)
 */
[[nodiscard]] inline auto buildUserValueRange(
    ScanDataType dataType, const std::vector<std::string>& args,
    size_t startIndex) -> std::optional<UserValueRange> {
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
