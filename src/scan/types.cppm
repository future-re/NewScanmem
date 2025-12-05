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
    ANY_NUMBER,   // matches any numeric type (integer or float)
    ANY_INTEGER,  // matches any integer type (8/16/32/64-bit)
    ANY_FLOAT,    // matches any float type (32/64-bit)
    INTEGER_8,    // 8-bit integer
    INTEGER_16,   // 16-bit integer
    INTEGER_32,   // 32-bit integer
    INTEGER_64,   // 64-bit integer
    FLOAT_32,     // 32-bit float
    FLOAT_64,     // 64-bit float
    BYTE_ARRAY,    // byte array
    STRING        // string
};

// Classification of match types
export enum class ScanMatchType {
    // Snapshot types (do not rely on user-provided values)
    MATCH_ANY,           // match any value (record snapshot)
    MATCH_UPDATE,        // equal to old value
    MATCH_NOT_CHANGED,   // equal to old value (equivalent to MATCH_UPDATE)
    MATCH_CHANGED,       // not equal to old value
    MATCH_INCREASED,     // greater than old value
    MATCH_DECREASED,     // less than old value
    MATCH_INCREASED_BY,  // current value - old value == user value
    MATCH_DECREASED_BY,  // old value - current value == user value

    // Comparisons with a user-provided value (requires a user value)
    MATCH_EQUAL_TO,      // equal to
    MATCH_NOT_EQUAL_TO,  // not equal to
    MATCH_GREATER_THAN,  // greater than
    MATCH_LESS_THAN,     // less than
    MATCH_RANGE,         // range [low, high]
    MATCH_REGEX          // regular expression (STRING only)
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
        case ScanDataType::INTEGER_8:
        case ScanDataType::INTEGER_16:
        case ScanDataType::INTEGER_32:
        case ScanDataType::INTEGER_64:
        case ScanDataType::FLOAT_32:
        case ScanDataType::FLOAT_64:
        case ScanDataType::ANY_INTEGER:
        case ScanDataType::ANY_FLOAT:
        case ScanDataType::ANY_NUMBER:
            return true;
        case ScanDataType::BYTE_ARRAY:
        case ScanDataType::STRING:
            return false;
    }
    return false;
}

export constexpr auto isAggregatedAny(ScanDataType scanDataType) noexcept
    -> bool {
    return scanDataType == ScanDataType::ANY_NUMBER ||
           scanDataType == ScanDataType::ANY_INTEGER ||
           scanDataType == ScanDataType::ANY_FLOAT;
}

export constexpr auto matchNeedsUserValue(ScanMatchType scanMatchType) noexcept
    -> bool {
    switch (scanMatchType) {
        case ScanMatchType::MATCH_EQUAL_TO:
        case ScanMatchType::MATCH_NOT_EQUAL_TO:
        case ScanMatchType::MATCH_GREATER_THAN:
        case ScanMatchType::MATCH_LESS_THAN:
        case ScanMatchType::MATCH_RANGE:
        case ScanMatchType::MATCH_REGEX:
        case ScanMatchType::MATCH_INCREASED_BY:
        case ScanMatchType::MATCH_DECREASED_BY:
            return true;
        default:
            return false;
    }
}

export constexpr auto matchUsesOldValue(ScanMatchType scanMatchType) noexcept
    -> bool {
    switch (scanMatchType) {
        case ScanMatchType::MATCH_UPDATE:
        case ScanMatchType::MATCH_NOT_CHANGED:
        case ScanMatchType::MATCH_CHANGED:
        case ScanMatchType::MATCH_INCREASED:
        case ScanMatchType::MATCH_DECREASED:
        case ScanMatchType::MATCH_INCREASED_BY:
        case ScanMatchType::MATCH_DECREASED_BY:
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
        case ScanDataType::INTEGER_8:
            return 1;
        case ScanDataType::INTEGER_16:
            return 2;
        case ScanDataType::INTEGER_32:
        case ScanDataType::FLOAT_32:
            return 4;
        case ScanDataType::INTEGER_64:
        case ScanDataType::FLOAT_64:
            return 8;
        case ScanDataType::STRING:
        case ScanDataType::BYTE_ARRAY:
            return 32;
        case ScanDataType::ANY_INTEGER:
        case ScanDataType::ANY_FLOAT:
        case ScanDataType::ANY_NUMBER:
            return 8;
    }
    return 8;
}
