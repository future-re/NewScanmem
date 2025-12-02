/**
 * @file memory_writer.cppm
 * @brief Memory writing abstraction using process_vm_writev
 */

module;

#include <sys/uio.h>
#include <unistd.h>

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <string>

export module core.memory_writer;

export namespace core {

/**
 * @struct WriteResult
 * @brief Result of a memory write operation
 */
struct WriteResult {
    std::uintptr_t address;
    std::size_t bytesWritten;
    bool success;
};

/**
 * @class MemoryWriter
 * @brief Handles memory writes to target process
 */
class MemoryWriter {
   public:
    explicit MemoryWriter(pid_t pid) : m_pid(pid) {}

    /**
     * @brief Write a value to memory at the specified address
     * @param address Target address in remote process
     * @param value Value to write (will be truncated to writeLen bytes)
     * @param writeLen Number of bytes to write (1, 2, 4, or 8)
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto write(std::uintptr_t address, std::uint64_t value,
                             std::size_t writeLen)
        -> std::expected<WriteResult, std::string> {
        if (writeLen == 0 || writeLen > 8) {
            return std::unexpected(
                std::format("Invalid write length: {}", writeLen));
        }

        // Prepare little-endian buffer
        std::array<std::uint8_t, 8> buf{};
        std::memcpy(buf.data(), &value, std::min(writeLen, buf.size()));

        iovec local{.iov_base = buf.data(), .iov_len = writeLen};
        iovec remote{.iov_base = std::bit_cast<void*>(address),
                     .iov_len = writeLen};

        ssize_t written = process_vm_writev(m_pid, &local, 1, &remote, 1, 0);

        if (written == static_cast<ssize_t>(writeLen)) {
            return WriteResult{
                .address = address, .bytesWritten = writeLen, .success = true};
        }

        return std::unexpected(
            std::format("Failed to write {} bytes to 0x{:016x} (wrote: {})",
                        writeLen, address, written));
    }

    /**
     * @brief Write a single byte to memory
     * @param address Target address
     * @param value Byte value
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto writeByte(std::uintptr_t address, std::uint8_t value)
        -> std::expected<WriteResult, std::string> {
        return write(address, value, 1);
    }

    /**
     * @brief Write bytes from a buffer to memory
     * @param address Target address
     * @param data Pointer to data buffer
     * @param size Number of bytes to write
     * @return Expected with WriteResult or error message
     */
    [[nodiscard]] auto writeBytes(std::uintptr_t address, const void* data,
                                  std::size_t size)
        -> std::expected<WriteResult, std::string> {
        if (data == nullptr || size == 0) {
            return std::unexpected("Invalid data or size");
        }

        iovec local{.iov_base = const_cast<void*>(data), .iov_len = size};
        iovec remote{.iov_base = std::bit_cast<void*>(address),
                     .iov_len = size};

        ssize_t written = process_vm_writev(m_pid, &local, 1, &remote, 1, 0);

        if (written == static_cast<ssize_t>(size)) {
            return WriteResult{
                .address = address, .bytesWritten = size, .success = true};
        }

        return std::unexpected(
            std::format("Failed to write {} bytes to 0x{:016x} (wrote: {})",
                        size, address, written));
    }

   private:
    pid_t m_pid;
};

}  // namespace core
