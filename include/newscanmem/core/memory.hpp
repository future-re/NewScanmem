#pragma once

/**
 * @file memory.hpp
 * @brief Memory writing interface wrapping proc.mem
 */

#include <unistd.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

#include "newscanmem/core/proc_mem.hpp"

namespace core {

/**
 * @class MemoryWriter
 * @brief High-level interface for writing to process memory
 */
class MemoryWriter {
   public:
    /**
     * @brief Construct memory writer for given process
     * @param pid Target process ID
     */
    explicit MemoryWriter(pid_t pid) : m_pid(pid) {}

    /**
     * @brief Write scalar value to memory address
     * @tparam T Scalar type (int, float, etc.)
     * @param addr Target address
     * @param value Value to write
     * @return Expected bytes written or error message
     */
    template <typename T>
    [[nodiscard]] auto write(void* addr, const T& value)
        -> std::expected<std::size_t, std::string> {
        return writeValue(m_pid, addr, value);
    }

    /**
     * @brief Write byte array to memory address
     * @param addr Target address
     * @param data Byte data to write
     * @return Expected bytes written or error message
     */
    [[nodiscard]] auto writeBytes(void* addr, std::span<const std::uint8_t> data) const
        -> std::expected<std::size_t, std::string>;

    /**
     * @brief Write C-style string to memory address
     * @param addr Target address
     * @param str String to write (null-terminated)
     * @return Expected bytes written or error message
     */
    [[nodiscard]] auto writeString(void* addr, const char* str) const
        -> std::expected<std::size_t, std::string>;

    /**
     * @brief Get target PID
     * @return Process ID
     */
    [[nodiscard]] auto getPid() const -> pid_t { return m_pid; }

   private:
    pid_t m_pid;
};

}  // namespace core
