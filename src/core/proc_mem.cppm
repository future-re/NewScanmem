/**
 * @file proc_mem.cppm
 * @brief Process memory I/O via /proc/<pid>/mem (进程内存 I/O)
 *
 * @internal
 * This is a low-level internal module intended for use by core.* modules only.
 * External code should use core.memory (MemoryWriter) for safer, higher-level
 * APIs.
 *
 * Provides minimal read/write capabilities using pread/pwrite on
 * /proc/<pid>/mem. Requirements:
 * - Sufficient privileges (root or CAP_SYS_PTRACE)
 * - Does NOT auto-attach/detach ptrace (caller manages debugging policy)
 * - Single fd open/reuse + one-shot convenience functions
 */

module;

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

export module core.proc_mem;

export namespace core {

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
    [[nodiscard]] auto open(bool writable) -> std::expected<void, std::string> {
        if (m_pid <= 0) {
            return std::unexpected{"invalid pid"};
        }
        int flags = O_CLOEXEC | (writable ? O_RDWR : O_RDONLY);
        std::string path = std::format("/proc/{}/mem", m_pid);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        int fileDesc = ::open(path.c_str(), flags);
        if (fileDesc < 0) {
            return std::unexpected{
                std::format("open {} failed: {}", path, std::strerror(errno))};
        }
        m_fd = fileDesc;
        return {};
    }

    ~ProcMemIO() {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
    }

    ProcMemIO(const ProcMemIO&) = delete;
    auto operator=(const ProcMemIO&) -> ProcMemIO& = delete;
    ProcMemIO(ProcMemIO&& other) noexcept
        : m_pid(other.m_pid), m_fd(other.m_fd) {
        other.m_fd = -1;
    }
    auto operator=(ProcMemIO&& other) noexcept -> ProcMemIO& {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        m_pid = other.m_pid;
        m_fd = other.m_fd;
        other.m_fd = -1;
        return *this;
    }

    /**
     * @brief Read bytes from target process memory
     * @param addr Target address in remote process
     * @param buf Buffer to store read data
     * @return Expected bytes read or error message (partial reads allowed)
     */
    [[nodiscard]] auto read(void* addr, std::span<std::uint8_t> buf) const
        -> std::expected<std::size_t, std::string> {
        if (m_fd < 0) {
            return std::unexpected{"not opened"};
        }
        auto off = static_cast<off_t>(std::bit_cast<std::uintptr_t>(addr));
        std::size_t total = 0;
        while (total < buf.size()) {
            ssize_t rval = ::pread(m_fd, buf.data() + total, buf.size() - total,
                                   static_cast<off_t>(off + total));
            if (rval < 0) {
                if (errno == EIO || errno == EFAULT || errno == EPERM ||
                    errno == EACCES) {
                    break;  // 返回部分成功
                }
                return std::unexpected{
                    std::format("pread error: {}", std::strerror(errno))};
            }
            if (rval == 0) {
                break;
            }
            total += static_cast<std::size_t>(rval);
        }
        return total;
    }

    /**
     * @brief Write bytes to target process memory
     * @param addr Target address in remote process
     * @param buf Data to write
     * @return Expected bytes written or error message
     */
    [[nodiscard]] auto write(void* addr,
                             std::span<const std::uint8_t> buf) const
        -> std::expected<std::size_t, std::string> {
        if (m_fd < 0) {
            return std::unexpected{"not opened"};
        }
        auto off = static_cast<off_t>(std::bit_cast<std::uintptr_t>(addr));
        std::size_t total = 0;
        while (total < buf.size()) {
            ssize_t rval =
                ::pwrite(m_fd, buf.data() + total, buf.size() - total,
                         static_cast<off_t>(off + total));
            if (rval < 0) {
                return std::unexpected{
                    std::format("pwrite error: {}", std::strerror(errno))};
            }
            if (rval == 0) {
                break;
            }
            total += static_cast<std::size_t>(rval);
        }
        return total;
    }

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
[[nodiscard]] inline auto writeBytes(pid_t pid, void* addr,
                                     std::span<const std::uint8_t> buf)
    -> std::expected<std::size_t, std::string> {
    ProcMemIO memIO{pid};
    if (auto err = memIO.open(true); !err) {
        return std::unexpected{err.error()};
    }
    return memIO.write(addr, buf);
}

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