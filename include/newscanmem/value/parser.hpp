#pragma once

/**
 * @file parser.hpp
 * @brief Value parsing utilities for scan operations
 */
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "newscanmem/scan/types.hpp"
#include "newscanmem/utils/parserStr.hpp"
#include "newscanmem/utils/string.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

namespace value {

// Forward declarations
template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalar(const std::vector<std::string>& args,
                                      size_t startIndex, Parser parser)
    -> std::optional<UserValue>;

[[nodiscard]] auto buildUserValueRange(
    ScanDataType dataType, const std::vector<std::string>& args,
    size_t startIndex) -> std::optional<UserValue>;

namespace detail {

[[nodiscard]] auto parseStringValue(const std::vector<std::string>& args,
                                    size_t startIndex)
    -> std::optional<UserValue>;

[[nodiscard]] auto parseByteArrayValue(const std::vector<std::string>& args,
                                       size_t startIndex)
    -> std::optional<UserValue>;

[[nodiscard]] auto parseAnyNumberValue(const std::vector<std::string>& args,
                                       size_t startIndex)
    -> std::optional<UserValue>;

}  // namespace detail

[[nodiscard]] auto parseDataType(std::string_view tok)
    -> std::optional<ScanDataType>;

[[nodiscard]] auto parseMatchType(std::string_view tok)
    -> std::optional<ScanMatchType>;

template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalar(const std::vector<std::string>& args,
                                      size_t startIndex, Parser parser)
    -> std::optional<UserValue> {
    auto valueOpt = parser(args[startIndex]);
    if (!valueOpt) {
        return std::nullopt;
    }
    return UserValue::fromScalar<T>(*valueOpt);
}

template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalarRange(const std::vector<std::string>& args,
                                           size_t startIndex, Parser parser)
    -> std::optional<UserValue> {
    auto valueLOpt = parser(args[startIndex]);
    if (!valueLOpt) {
        return std::nullopt;
    }
    auto valueROpt = parser(args[startIndex + 1]);
    if (!valueROpt) {
        return std::nullopt;
    }
    return UserValue{Value::of<T>(*valueLOpt), Value::of<T>(*valueROpt)};
}

[[nodiscard]] auto buildUserValue(
    ScanDataType dataType, ScanMatchType matchType,
    const std::vector<std::string>& args,
    size_t startIndex) -> std::optional<UserValue>;

}  // namespace value
