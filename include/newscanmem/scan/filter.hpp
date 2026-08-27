#pragma once

#include <expected>
#include <string>

#include "newscanmem/scan/match_storage.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"

[[nodiscard]] auto filterMatches(pid_t pid, const ScanOptions& options, const UserValue* value,
                                 scan::MatchesAndOldValuesArray& matches) -> std::expected<ScanStats, std::string>;
