#include "newscanmem/value/flags.hpp"

void setFlagsIfNotNull(MatchFlags* destination, const MatchFlags flags) noexcept {
  if (destination != nullptr) *destination = flags;
}

void orFlagsIfNotNull(MatchFlags* destination, const MatchFlags flags) noexcept {
  if (destination != nullptr) *destination |= flags;
}
