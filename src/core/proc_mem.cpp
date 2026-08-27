#include "newscanmem/core/proc_mem.hpp"

namespace core {
auto ProcMemIO::open(const bool writable) -> std::expected<void, std::string> {
    if (m_pid <= 0) return std::unexpected("invalid pid");
    const auto path = std::format("/proc/{}/mem", m_pid);
    const int descriptor =
        ::open(path.c_str(), O_CLOEXEC | (writable ? O_RDWR : O_RDONLY));
    if (descriptor < 0)
        return std::unexpected(
            std::format("open {} failed: {}", path, std::strerror(errno)));
    if (m_fd >= 0) ::close(m_fd);
    m_fd = descriptor;
    return {};
}
auto ProcMemIO::open() -> std::expected<void, std::string> {
    return open(false);
}
ProcMemIO::~ProcMemIO() {
    if (m_fd >= 0) ::close(m_fd);
}
ProcMemIO::ProcMemIO(ProcMemIO&& other) noexcept
    : m_pid(other.m_pid), m_fd(other.m_fd) {
    other.m_fd = -1;
}
auto ProcMemIO::operator=(ProcMemIO&& other) noexcept -> ProcMemIO& {
    if (this != &other) {
        if (m_fd >= 0) ::close(m_fd);
        m_pid = other.m_pid;
        m_fd = other.m_fd;
        other.m_fd = -1;
    }
    return *this;
}
auto ProcMemIO::read(void* address, const std::span<std::uint8_t> buffer) const
    -> std::expected<std::size_t, std::string> {
    if (m_fd < 0) return std::unexpected("not opened");
    const auto offset =
        static_cast<off_t>(std::bit_cast<std::uintptr_t>(address));
    std::size_t total = 0;
    while (total < buffer.size()) {
        const auto readSize =
            ::pread(m_fd, buffer.data() + total, buffer.size() - total,
                    offset + static_cast<off_t>(total));
        if (readSize < 0) {
            if (errno == EINTR) continue;
            if (errno == EIO || errno == EFAULT) break;
            return std::unexpected(
                std::format("pread error: {}", std::strerror(errno)));
        }
        if (readSize == 0) break;
        total += static_cast<std::size_t>(readSize);
    }
    return total;
}
auto ProcMemIO::read(void* address, std::uint8_t* buffer,
                     const std::size_t size) const
    -> std::expected<std::size_t, std::string> {
    return read(address, std::span{buffer, size});
}
auto ProcMemIO::write(void* address, const std::span<const std::uint8_t> buffer)
    const -> std::expected<std::size_t, std::string> {
    if (m_fd < 0) return std::unexpected("not opened");
    const auto offset =
        static_cast<off_t>(std::bit_cast<std::uintptr_t>(address));
    std::size_t total = 0;
    while (total < buffer.size()) {
        const auto written =
            ::pwrite(m_fd, buffer.data() + total, buffer.size() - total,
                     offset + static_cast<off_t>(total));
        if (written < 0) {
            if (errno == EINTR) continue;
            return std::unexpected(
                std::format("pwrite error: {}", std::strerror(errno)));
        }
        if (written == 0) break;
        total += static_cast<std::size_t>(written);
    }
    return total;
}
auto writeBytes(const pid_t pid, void* address,
                const std::span<const std::uint8_t> buffer)
    -> std::expected<std::size_t, std::string> {
    ProcMemIO memory{pid};
    if (const auto opened = memory.open(true); !opened)
        return std::unexpected(opened.error());
    return memory.write(address, buffer);
}
}  // namespace core
