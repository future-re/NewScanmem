module;

#include <cstddef>
#include <functional>

export module scan.routine;

import value;

// Match routine signature for a single scan location. Returns the number of
// matched bytes (>= 1) or 0 (no match).
export using scanRoutine = std::function<unsigned int(
    const Value* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;
