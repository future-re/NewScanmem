#include "memseek/utils/endianness.hpp"

namespace utils {

auto getHost() -> Endianness {
    return isLittleEndian() ? Endianness::LITTLE : Endianness::BIG;
}

}  // namespace utils
