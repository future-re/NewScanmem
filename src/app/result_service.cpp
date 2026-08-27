#include "newscanmem/app/result_service.hpp"

namespace app {
auto ResultService::getMatches(const CurrentMatchListRequest& request)
    -> std::expected<std::pair<std::vector<core::MatchEntry>, std::size_t>, std::string> {
  if (request.scanner == nullptr) return std::unexpected("No scanner initialized. Run a scan first.");
  auto classified = core::RegionClassifier::create(request.pid);
  std::optional<core::RegionClassifier> classifier;
  if (classified) classifier = std::move(*classified);
  core::MatchCollector collector{std::move(classifier)};
  return collector.collect({.matches = &request.scanner->getMatches(), .dataType = request.scanner->getLastDataType()},
                           {.limit = request.limit, .collectRegion = request.showRegion});
}
}  // namespace app
