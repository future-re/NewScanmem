#pragma once

/**
 * @file proc_mem.hpp
 * @brief Process memory I/O via /proc/<pid>/mem (进程内存 I/O)
 *
 * Unified low-level read/write interface for target process memory.
 * Used by core.scanner, scan.engine, and scan.filter.
 *
 * Provides minimal read/write capabilities using pread/pwrite on
 * /proc/<pid>/mem. Requirements:
 * - Sufficient privileges (root or CAP_SYS_PTRACE)
 * - Does NOT auto-attach/detach ptrace (caller manages debugging policy)
 * - Single fd open/reuse + one-shot convenience functions
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <span>
#include <string>

namespace core {

/**
 * @class ProcMemIO
 * @brief RAII wrapper for /proc/<pid>/mem file descriptor
 *
 * Manages a single fd for reading/writing target process memory.
 * Caller must handle ptrace attach/detach externally if needed.
 */
class ProcMemIO {
   public:
    ProcMemIO() = default;
    explicit ProcMemIO(pid_t pid) : m_pid(pid) {}

    /**
     * @brief Open /proc/<pid>/mem with specified mode
     * @param writable If true, opens O_RDWR; otherwise O_RDONLY
     * @return Expected void or error message
     */
    [[nodiscard]] auto open(bool writable) -> std::expected<void, std::string>;

    /**
     * @brief Open /proc/<pid>/mem in read-only mode (convenience overload)
     * @return Expected void or error message
     */
    [[nodiscard]] auto open() -> std::expected<void, std::string>;

    ~ProcMemIO();

    ProcMemIO(const ProcMemIO&) = delete;
    auto operator=(const ProcMemIO&) -> ProcMemIO& = delete;
    ProcMemIO(ProcMemIO&& other) noexcept;
    auto operator=(ProcMemIO&& other) noexcept -> ProcMemIO&;

    /**
     * @brief Read bytes from target process memory
     * @param addr Target address in remote process
     * @param buf Buffer to store read data
     * @return Expected bytes read or error message (partial reads allowed)
     */
    [[nodiscard]] auto read(void* addr, std::span<std::uint8_t> buf) const
        -> std::expected<std::size_t, std::string>;

    /**
     * @brief Read bytes from target process memory (raw pointer overload)
     * @param addr Target address in remote process
     * @param buf Buffer to store read data
     * @param len Number of bytes to read
     * @return Expected bytes read or error message
     */
    [[nodiscard]] auto read(void* addr, std::uint8_t* buf, std::size_t len)
        const -> std::expected<std::size_t, std::string>;

    /**
     * @brief Write bytes to target process memory
     * @param addr Target address in remote process
     * @param buf Data to write
     * @return Expected bytes written or error message
     */
    [[nodiscard]] auto write(void* addr, std::span<const std::uint8_t> buf)
        const -> std::expected<std::size_t, std::string>;

    /**
     * @brief Write scalar value to target memory
     * @tparam T Trivially copyable type (int, float, etc.)
     * @param addr Target address
     * @param value Value to write
     * @return Expected bytes written or error message
     */
    template <typename T>
    [[nodiscard]] auto writeScalar(void* addr, const T& value)
        -> std::expected<std::size_t, std::string> {
        static_assert(std::is_trivially_copyable_v<T>);
        auto bytes = std::as_bytes(std::span{&value, 1});
        return write(addr, std::span<const std::uint8_t>{
                               std::bit_cast<const std::uint8_t*>(bytes.data()),
                               bytes.size()});
    }

   private:
    pid_t m_pid{-1};
    int m_fd{-1};
};

/**
 * @brief One-shot write: opens, writes, and closes /proc/<pid>/mem
 * @param pid Target process ID
 * @param addr Target address
 * @param buf Data to write
 * @return Expected bytes written or error message
 */
[[nodiscard]] auto writeBytes(pid_t pid, void* addr,
                              std::span<const std::uint8_t> buf)
    -> std::expected<std::size_t, std::string>;

/**
 * @brief One-shot write scalar value
 * @tparam T Trivially copyable type
 * @param pid Target process ID
 * @param addr Target address
 * @param value Value to write
 * @return Expected bytes written or error message
 */
template <typename T>
[[nodiscard]] inline auto writeValue(pid_t pid, void* addr, const T& value)
    -> std::expected<std::size_t, std::string> {
    static_assert(std::is_trivially_copyable_v<T>);
    ProcMemIO memIO{pid};
    if (auto err = memIO.open(true); !err) {
        return std::unexpected{err.error()};
    }
    return memIO.writeScalar<T>(addr, value);
}

}  // namespace core
