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

// 进程内存 I/O：最小可用写入能力（poke）+ 读功能
// 说明：
// - 采用 /proc/<pid>/mem + pread/pwrite 实现；需要足够权限（root 或
// CAP_SYS_PTRACE）。
// - 未自动 ptrace attach/detach（与调试器共享策略交给上层）。
// - 单文件打开/复用 fd；也提供一次性写入的便捷函数。

export class ProcMemIO {
   public:
    ProcMemIO() = default;
    explicit ProcMemIO(pid_t pid) : m_pid(pid) {}

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

// 便捷一次性写入：内部打开/关闭
export [[nodiscard]] inline auto writeBytes(pid_t pid, void* addr,
                                            std::span<const std::uint8_t> buf)
    -> std::expected<std::size_t, std::string> {
    ProcMemIO memIO{pid};
    if (auto err = memIO.open(true); !err) {
        return std::unexpected{err.error()};
    }
    return memIO.write(addr, buf);
}

export template <typename T>
[[nodiscard]] inline auto writeValue(pid_t pid, void* addr, const T& value)
    -> std::expected<std::size_t, std::string> {
    static_assert(std::is_trivially_copyable_v<T>);
    ProcMemIO memIO{pid};
    if (auto err = memIO.open(true); !err) {
        return std::unexpected{err.error()};
    }
    return memIO.writeScalar<T>(addr, value);
}