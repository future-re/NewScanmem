# Sets Module Documentation

## Overview

The `sets` module provides set operations and parsing utilities for the NewScanmem project. It includes a `Set` class for managing collections of integers and a powerful parser for set expressions with support for ranges, hex/decimal numbers, and inversion operations.

## Module Structure

```cpp
export module sets;
```

## Dependencies

- `<algorithm>` - Standard algorithms
- `<boost/regex.hpp>` - Regular expression support
- `<boost/spirit/include/phoenix.hpp>` - Boost Spirit Phoenix for semantic actions
- `<boost/spirit/include/qi.hpp>` - Boost Spirit Qi for parsing
- `<cctype>` - Character classification
- `<compare>` - Three-way comparison
- `<cstdlib>` - C standard library
- `<stdexcept>` - Standard exceptions
- `<string>` - String operations
- `<vector>` - Dynamic array container

## Core Features

### 1. Set Structure

```cpp
export struct Set {
    std::vector<size_t> buf;
    
    size_t size() const;
    void clear();
    static int cmp(const size_t& i1, const size_t& i2);
};
```

#### Methods

- **size()**: Returns the number of elements in the set
- **clear()**: Removes all elements from the set
- **cmp()**: Static comparison function for two size_t values using three-way comparison

### 2. Set Expression Parser

```cpp
export bool parse_uintset(std::string_view lptr, Set& set, size_t maxSZ);
```

#### Supported Expression Formats

- **Single numbers**: `42`, `0x2A`
- **Ranges**: `10..20`, `0x10..0xFF`
- **Multiple values**: `1,2,3,4,5`
- **Mixed format**: `1,5,10..15,0x20`
- **Inversion**: `!1,2,3` (all numbers except 1,2,3)
- **Hexadecimal**: `0x10`, `0xFF`, `0xdeadbeef`

#### Parameters

- **lptr**: The set expression string to parse
- **set**: The Set object to populate with results
- **maxSZ**: Maximum allowed value (exclusive upper bound)

#### Return Value

- `true`: Parsing successful
- `false`: Parsing failed (invalid syntax, out of bounds, etc.)

### 3. Deprecated Memory Management

```cpp
[[deprecated("This interface is deprecated...")]]
constexpr auto inc_arr_sz = [](size_t** valarr, size_t* arr_maxsz, size_t maxsz) -> bool;
```

A deprecated C-style memory management utility for dynamic array resizing.

## Usage Examples

### Basic Set Parsing

```cpp
import sets;

Set mySet;
bool success = parse_uintset("1,2,3,4,5", mySet, 100);
if (success) {
    std::cout << "Set contains " << mySet.size() << " elements\n";
}
```

### Range Parsing

```cpp
Set rangeSet;
parse_uintset("10..20", rangeSet, 100);
// Results: {10, 11, 12, ..., 20}
```

### Hexadecimal Support

```cpp
Set hexSet;
parse_uintset("0x10,0x20,0x30..0x35", hexSet, 256);
// Results: {16, 32, 48, 49, 50, 51, 52, 53}
```

### Set Inversion

```cpp
Set invertedSet;
parse_uintset("!0,1,2", invertedSet, 10);
// Results: {3, 4, 5, 6, 7, 8, 9}
```

### Complex Expressions

```cpp
Set complexSet;
parse_uintset("0,5,10..15,0x20,!12", complexSet, 100);
// Results: {0, 5, 10, 11, 13, 14, 15, 32}
```

### Empty Set Handling

```cpp
Set emptySet;
bool result = parse_uintset("", emptySet, 100);  // Returns false

Set invertedEmpty;
parse_uintset("!", invertedEmpty, 5);  // Results: {0, 1, 2, 3, 4}
```

## Parser Grammar

The parser uses Boost Spirit Qi and supports the following grammar:

```text
expression ::= ["!"] (range | single) { "," (range | single) }
range ::= number ".." number
single ::= number
number ::= hex_number | decimal_number
hex_number ::= "0x" hex_digits
decimal_number ::= decimal_digits
```

## Error Handling

### Parse Errors

The parser will return `false` for:

- Invalid syntax (e.g., "1..", "abc", "1..2..3")
- Out of bounds values (exceeding maxSZ)
- Empty expressions (except for inversion of empty set)
- Invalid ranges (start > end)

### Exception Safety

The parser uses exception handling internally but converts all Boost Spirit exceptions to boolean return values for clean API usage.

## Performance Considerations

- **Complexity**: O(n log n) due to sorting and duplicate removal
- **Memory**: Uses std::vector for storage with automatic memory management
- **Boost Spirit**: Parser is efficient but has some compile-time overhead

## Implementation Details

### Sorting and Deduplication

After parsing, the module automatically:

1. Sorts the elements in ascending order
2. Removes duplicate values
3. Handles inversion logic efficiently

### Range Expansion

Ranges are expanded into individual values:

- `10..15` becomes `{10, 11, 12, 13, 14, 15}`
- Each value is validated against maxSZ

### Inversion Logic

Inversion creates the complement set:

- Original: `{1, 3, 5}` with maxSZ=10
- Inverted: `{0, 2, 4, 6, 7, 8, 9}`

## Limitations

- **maxSZ**: All values must be less than maxSZ
- **Memory**: Large ranges can consume significant memory
- **Performance**: Very large ranges may impact performance

## Examples of Invalid Input

```cpp
Set set;
parse_uintset("5..2", set, 100);     // false - invalid range
parse_uintset("1,200", set, 100);    // false - out of bounds
parse_uintset("abc", set, 100);      // false - invalid syntax
parse_uintset("1..", set, 100);      // false - incomplete range
```

## Testing

```cpp
#include <iostream>
#include <set>

void test_parser() {
    Set set;
    
    // Test basic parsing
    parse_uintset("1,2,3", set, 100);
    assert(set.size() == 3);
    
    // Test ranges
    parse_uintset("1..3", set, 100);
    assert(set.size() == 3);
    
    // Test inversion
    parse_uintset("!1..3", set, 10);
    assert(set.size() == 7);
    
    std::cout << "All tests passed!\n";
}
```

## See Also

- [Main Application](main.md) - For integration examples
- [Value Module](value.md) - For value type definitions used with sets

## Future Enhancements

- Support for floating-point ranges
- Custom delimiters
- Performance optimizations for large ranges
- Memory-efficient sparse set representation
