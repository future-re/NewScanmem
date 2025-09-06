module;

#include <cstddef>
#include <functional>

export module scanroutines;

import value;

enum class ScanDataType {
    ANYNUMBER,  /* ANYINTEGER or ANYFLOAT */
    ANYINTEGER, /* INTEGER of whatever width */
    ANYFLOAT,   /* FLOAT of whatever width */
    INTEGER8,
    INTEGER16,
    INTEGER32,
    INTEGER64,
    FLOAT32,
    FLOAT64,
    BYTEARRAY,
    STRING
};

enum class ScanMatchType {
    MATCHANY, /* for snapshot */
    /* following: compare with a given value */
    MATCHEQUALTO,
    MATCHNOTEQUALTO,
    MATCHGREATERTHAN,
    MATCHLESSTHAN,
    MATCHRANGE,
    /* following: compare with the old value */
    MATCHUPDATE,
    MATCHNOTCHANGED,
    MATCHCHANGED,
    MATCHINCREASED,
    MATCHDECREASED,
    /* following: compare with both given value and old value */
    MATCHINCREASEDBY,
    MATCHDECREASEDBY
};

using scanRoutine = std::function<unsigned int(
    const Mem64* /*memoryPtr*/, size_t /*memLength*/, const Value* /*oldValue*/,
    const UserValue* /*userValue*/, MatchFlags* /*saveFlags*/)>;


/* for convenience */
#define SCAN_ROUTINE_ARGUMENTS (const mem64_t *memory_ptr, size_t memlength, const value_t *old_value, const uservalue_t *user_value, match_flags *saveflags)
unsigned int (*sm_scan_routine) SCAN_ROUTINE_ARGUMENTS;

#define MEMORY_COMP(value,field,op)  (((value)->flags & flag_##field) && (get_##field(memory_ptr) op get_##field(value)))
#define GET_FLAG(valptr, field)      ((valptr)->flags & flag_##field)
#define SET_FLAG(flagsptr, field)    ((*(flagsptr)) |= flag_##field)


auto smChooseScanroutine(ScanDataType dataType, ScanMatchType matchType,
                         UserValue& userValue, bool reverseEndianness) -> bool {

}

auto smGetScanroutine(ScanDataType dataType, ScanMatchType matchType,
                      MatchFlags uflags, bool reverseEndianness) -> scanRoutine;


