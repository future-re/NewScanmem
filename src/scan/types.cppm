module;

#include <cstddef>
#include <functional>

export module scan.types;

// Import dependent modules and re-export their symbols to avoid redeclaring
// types with the same names across modules.
export import utils.mem64;  // provides Mem64
export import value.flags;  // provides MatchFlags
export import value;  // provides Value / UserValue, etc. (aggregated in value)

// Classification of scan data types
export enum class ScanDataType {
    ANYNUMBER,   // matches any numeric type (integer or float)
    ANYINTEGER,  // matches any integer type (8/16/32/64-bit)
    ANYFLOAT,    // matches any float type (32/64-bit)
    INTEGER8,    // 8-bit integer
    INTEGER16,   // 16-bit integer
    INTEGER32,   // 32-bit integer
    INTEGER64,   // 64-bit integer
    FLOAT32,     // 32-bit float
    FLOAT64,     // 64-bit float
    BYTEARRAY,   // byte array
    STRING       // string
};

// Classification of match types
export enum class ScanMatchType {
    // Snapshot types (do not rely on user-provided values)
    MATCHANY,          // match any value (record snapshot)
    MATCHUPDATE,       // equal to old value
    MATCHNOTCHANGED,   // equal to old value (equivalent to MATCHUPDATE)
    MATCHCHANGED,      // not equal to old value
    MATCHINCREASED,    // greater than old value
    MATCHDECREASED,    // less than old value
    MATCHINCREASEDBY,  // current value - old value == user value
    MATCHDECREASEDBY,  // old value - current value == user value

    // Comparisons with a user-provided value (requires a user value)
    MATCHEQUALTO,      // equal to
    MATCHNOTEQUALTO,   // not equal to
    MATCHGREATERTHAN,  // greater than
    MATCHLESSTHAN,     // less than
    MATCHRANGE,        // range [low, high]
    MATCHREGEX         // regular expression (STRING only)
};

// Match routine signature for a single scan location. Returns the number of
// matched bytes (>= 1) or 0 (no match).
export using scanRoutine = std::function<unsigned int(
    const Mem64* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;

// Byte pattern search result (with offset and length), useful when marking
// target memory or ranges.
export struct ByteMatch {
    size_t offset{};
    size_t length{};
};

// Common utilities (type-level convenience checks that introduce no
// implementation dependencies)
export constexpr auto isNumericType(ScanDataType scanDataType) noexcept
    -> bool {
    switch (scanDataType) {
        case ScanDataType::INTEGER8:
        case ScanDataType::INTEGER16:
        case ScanDataType::INTEGER32:
        case ScanDataType::INTEGER64:
        case ScanDataType::FLOAT32:
        case ScanDataType::FLOAT64:
        case ScanDataType::ANYINTEGER:
        case ScanDataType::ANYFLOAT:
        case ScanDataType::ANYNUMBER:
            return true;
        case ScanDataType::BYTEARRAY:
        case ScanDataType::STRING:
            return false;
    }
    return false;
}

export constexpr auto isAggregatedAny(ScanDataType scanDataType) noexcept
    -> bool {
    return scanDataType == ScanDataType::ANYNUMBER ||
           scanDataType == ScanDataType::ANYINTEGER ||
           scanDataType == ScanDataType::ANYFLOAT;
}

export constexpr auto matchNeedsUserValue(ScanMatchType scanMatchType) noexcept
    -> bool {
    switch (scanMatchType) {
        case ScanMatchType::MATCHEQUALTO:
        case ScanMatchType::MATCHNOTEQUALTO:
        case ScanMatchType::MATCHGREATERTHAN:
        case ScanMatchType::MATCHLESSTHAN:
        case ScanMatchType::MATCHRANGE:
        case ScanMatchType::MATCHREGEX:
        case ScanMatchType::MATCHINCREASEDBY:
        case ScanMatchType::MATCHDECREASEDBY:
            return true;
        default:
            return false;
    }
}

export constexpr auto matchUsesOldValue(ScanMatchType scanMatchType) noexcept
    -> bool {
    switch (scanMatchType) {
        case ScanMatchType::MATCHUPDATE:
        case ScanMatchType::MATCHNOTCHANGED:
        case ScanMatchType::MATCHCHANGED:
        case ScanMatchType::MATCHINCREASED:
        case ScanMatchType::MATCHDECREASED:
        case ScanMatchType::MATCHINCREASEDBY:
        case ScanMatchType::MATCHDECREASEDBY:
            return true;
        default:
            return false;
    }
}

// Helper: minimal bytes needed for a type (shared with scanning and filtering
// logic)
export [[nodiscard]] constexpr auto bytesNeededForType(ScanDataType dataType)
    -> std::size_t {
    switch (dataType) {
        case ScanDataType::INTEGER8:
            return 1;
        case ScanDataType::INTEGER16:
            return 2;
        case ScanDataType::INTEGER32:
        case ScanDataType::FLOAT32:
            return 4;
        case ScanDataType::INTEGER64:
        case ScanDataType::FLOAT64:
            return 8;
        case ScanDataType::STRING:
        case ScanDataType::BYTEARRAY:
            return 32;
        case ScanDataType::ANYINTEGER:
        case ScanDataType::ANYFLOAT:
        case ScanDataType::ANYNUMBER:
            return 8;
    }
    return 8;
}
