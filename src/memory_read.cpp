#include "memseek/memory_read.hpp"

#include <cerrno>
#include <cstring>
#include <expected>
#include <string>

#if defined(__linux__)
#include <sys/uio.h>
#include <unistd.h>
#endif

namespace memseek {

std::expected<MemoryRead, std::string> MemoryRead::readMemory(
    pid_t pid, const MemoryRegion& region) {
#if defined(__linux__)

    MemoryRead result{region.start, region.size};

    iovec local{
        .iov_base = result.m_buffer.data(),
        .iov_len = result.m_buffer.size(),
    };

    iovec remote{
        .iov_base = reinterpret_cast<void*>(region.start),
        .iov_len = region.size,   
    };

    const ssize_t bytesRead = ::process_vm_readv(pid, &local, 1, &remote, 1, 0);

    if (bytesRead < 0) {
        return std::unexpected{std::string{"process_vm_readv failed: "} +
                               std::strerror(errno)};
    }

    result.m_buffer.resize(static_cast<std::size_t>(bytesRead));

    return result;

#else

    return std::unexpected{
        "MemoryRead::readMemory is not supported on this platform"};

#endif
}
}  // namespace memseek