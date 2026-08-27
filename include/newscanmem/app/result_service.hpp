#pragma once

/**
 * @file result_service.hpp
 * @brief Thin application service for current match presentation
 */

#include <sys/types.h>

#include <bit>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "newscanmem/core/match.hpp"
#include "newscanmem/core/region_classifier.hpp"
#include "newscanmem/core/scanner.hpp"
#include "newscanmem/utils/endianness.hpp"

namespace app {

struct CurrentMatchListRequest {
    const core::Scanner* scanner{nullptr};
    pid_t pid{0};
    std::size_t limit{20};
    bool showRegion{true};
    bool showIndex{true};
    utils::Endianness endianness{(std::endian::native == std::endian::little
                                      ? utils::Endianness::LITTLE
                                      : utils::Endianness::BIG)};
};

class ResultService {
   public:
    /**
     * @brief Get current match entries from scanner
     * @param request Request parameters
     * @return Pair of (entries, total_count) or error
     */
    [[nodiscard]] static auto getMatches(const CurrentMatchListRequest& request)
        -> std::expected<std::pair<std::vector<core::MatchEntry>, std::size_t>, std::string>;
};

}  // namespace app
