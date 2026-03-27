/**
 * @file result_service.cppm
 * @brief Thin application service for current match presentation
 */

module;

#include <sys/types.h>

#include <bit>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>

export module app.result_service;

import core.match;
import core.match_formatter;
import core.region_classifier;
import core.scanner;
import ui.show_message;
import utils.endianness;

export namespace app {

struct CurrentMatchListRequest {
    const core::Scanner* scanner{nullptr};
    pid_t pid{0};
    std::size_t limit{20};
    bool showRegion{true};
    bool showIndex{true};
    utils::Endianness endianness{
        (std::endian::native == std::endian::little
             ? utils::Endianness::LITTLE
             : utils::Endianness::BIG)};
};

class ResultService {
   public:
    [[nodiscard]] static auto showCurrentMatches(
        const CurrentMatchListRequest& request)
        -> std::expected<std::size_t, std::string> {
        if (request.scanner == nullptr) {
            return std::unexpected("No scanner initialized. Run a scan first.");
        }

        auto classifierResult = core::RegionClassifier::create(request.pid);
        std::optional<core::RegionClassifier> classifier;
        if (classifierResult) {
            classifier = std::move(*classifierResult);
        }

        core::MatchCollector collector{std::move(classifier)};
        core::MatchCollectionOptions collectOptions{
            .limit = request.limit, .collectRegion = request.showRegion};

        auto [entries, totalCount] =
            collector.collect(*request.scanner, collectOptions);

        core::FormatOptions formatOptions{
            .showRegion = request.showRegion,
            .showIndex = request.showIndex,
            .bigEndianDisplay =
                (request.endianness == utils::Endianness::BIG),
            .dataType = request.scanner->getLastDataType()};
        core::MatchFormatter::display(entries, totalCount, formatOptions);

        return totalCount;
    }
};

}  // namespace app
