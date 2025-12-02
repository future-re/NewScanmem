import core.memory;
import core.proc_mem; // For direct read-back verification

using core::MemoryWriter;
using core::ProcMemIO;

#include <errno.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cstring>
#include <thread>

/**
 * @brief Test fixture for memory write operations
 *
 * Creates a child process with a known memory region that can be modified
 * from the parent process using MemoryWriter.
 */
// Global pipe write fd used by child after fork
static int g_pipe_write_fd = -1;

class MemoryWriteTest : public ::testing::Test {
   protected:
    template <typename T>
    [[nodiscard]] static auto readScalar(pid_t pid, void* addr)
        -> std::optional<T> {
        ProcMemIO memIO(pid);
        auto openRes = memIO.open(false);
        if (!openRes) {
            return std::nullopt;
        }
        T value{};
        auto bytes = std::as_bytes(std::span{&value, 1});
        auto readRes = memIO.read(
            addr,
            std::span<std::uint8_t>{std::bit_cast<std::uint8_t*>(bytes.data()),
                                    bytes.size()});
        if (!readRes || *readRes != sizeof(T)) {
            return std::nullopt;
        }
        return value;
    }
    [[nodiscard]] auto childPid() const -> pid_t { return m_childPid; }
    [[nodiscard]] auto intAddr() const -> void* { return m_intAddr; }
    [[nodiscard]] auto floatAddr() const -> void* { return m_floatAddr; }
    [[nodiscard]] auto uint64Addr() const -> void* { return m_uint64Addr; }
    [[nodiscard]] auto bufferAddr() const -> void* { return m_bufferAddr; }

    void SetUp() override {
        int pipefd[2];
        ASSERT_EQ(::pipe(pipefd), 0) << "pipe creation failed";
        g_pipe_write_fd = pipefd[1];

        m_childPid = fork();
        if (m_childPid == 0) {
            // Child process
            ::close(pipefd[0]);  // close read end
            runChildProcess();
            _exit(0);
        } else if (m_childPid < 0) {
            FAIL() << "Failed to fork child process";
        }
        // Parent
        ::close(pipefd[1]);  // close write end in parent
        // Read 4 pointers from child
        std::array<uintptr_t, 4> addrs{};
        std::size_t need = addrs.size() * sizeof(uintptr_t);
        std::size_t got = 0;
        while (got < need) {
            ssize_t r =
                ::read(pipefd[0], reinterpret_cast<char*>(addrs.data()) + got,
                       need - got);
            ASSERT_GE(r, 0) << "read failed: " << strerror(errno);
            if (r == 0) break;
            got += static_cast<std::size_t>(r);
        }
        ASSERT_EQ(got, need) << "Did not receive all addresses from child";
        ::close(pipefd[0]);
        m_intAddr = reinterpret_cast<void*>(addrs[0]);
        m_floatAddr = reinterpret_cast<void*>(addrs[1]);
        m_uint64Addr = reinterpret_cast<void*>(addrs[2]);
        m_bufferAddr = reinterpret_cast<void*>(addrs[3]);
    }

    void TearDown() override {
        if (m_childPid > 0) {
            // Kill child process
            kill(m_childPid, SIGTERM);
            waitpid(m_childPid, nullptr, 0);
        }
    }

    /**
     * @brief Child process target: maintains a test variable in memory
     */
    static void runChildProcess() {
        // Allocate a dedicated page for test block
        const size_t pageSize = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
        void* block = ::mmap(nullptr, pageSize, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (block == MAP_FAILED) {
            _exit(1);
        }
        struct TestBlock {
            volatile int testInt;
            volatile float testFloat;
            volatile uint64_t testUint64;
            volatile char buffer[64];
        };
        auto* tb = reinterpret_cast<TestBlock*>(block);
        tb->testInt = 42;
        tb->testFloat = 3.14F;
        tb->testUint64 = 0x1234567890ABCDEFULL;
        std::memset(const_cast<char*>(tb->buffer), 0, sizeof(tb->buffer));

        // Send addresses to parent
        std::array<uintptr_t, 4> out{
            reinterpret_cast<uintptr_t>(&tb->testInt),
            reinterpret_cast<uintptr_t>(&tb->testFloat),
            reinterpret_cast<uintptr_t>(&tb->testUint64),
            reinterpret_cast<uintptr_t>(tb->buffer)};
        ssize_t w = ::write(g_pipe_write_fd, out.data(),
                            out.size() * sizeof(uintptr_t));
        (void)w;
        // Keep process alive and touch values
        while (true) {
            tb->testInt = tb->testInt;
            tb->testFloat = tb->testFloat;
            tb->testUint64 = tb->testUint64;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    // Scanning helpers removed: we use deterministic mmap addresses now.

   private:
    pid_t m_childPid = -1;
    void* m_intAddr = nullptr;
    void* m_floatAddr = nullptr;
    void* m_uint64Addr = nullptr;
    void* m_bufferAddr = nullptr;
};

// Test: Write integer value
TEST_F(MemoryWriteTest, WriteIntValue) {
    ASSERT_GT(childPid(), 0) << "Child process should be created";

    // Use provided deterministic address
    void* targetAddr = intAddr();
    ASSERT_NE(targetAddr, nullptr) << "Int address should be available";

    // Create memory writer
    MemoryWriter writer(childPid());

    // Write new value
    int newValue = 100;
    auto result = writer.write(targetAddr, newValue);
    ASSERT_TRUE(result.has_value()) << "Write should succeed";
    EXPECT_EQ(*result, sizeof(int)) << "Should write 4 bytes";

    // Precise verification: read back the int at targetAddr
    auto readBack = readScalar<int>(childPid(), targetAddr);
    ASSERT_TRUE(readBack.has_value()) << "Read-back failed";
    EXPECT_EQ(*readBack, newValue) << "Memory value should be updated";
}

// Test: Write float value
TEST_F(MemoryWriteTest, WriteFloatValue) {
    ASSERT_GT(childPid(), 0);
    void* targetAddr = floatAddr();
    ASSERT_NE(targetAddr, nullptr);

    MemoryWriter writer(childPid());
    float newValue = 6.28F;
    auto result = writer.write(targetAddr, newValue);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, sizeof(float));

    // Read-back verification
    auto readBack = readScalar<float>(childPid(), targetAddr);
    ASSERT_TRUE(readBack.has_value());
    EXPECT_FLOAT_EQ(*readBack, newValue);
}

// Test: Write byte array
TEST_F(MemoryWriteTest, WriteByteArray) {
    ASSERT_GT(childPid(), 0);
    // Use a known 64-bit value location
    void* targetAddr = uint64Addr();
    ASSERT_NE(targetAddr, nullptr);

    MemoryWriter writer(childPid());
    std::vector<uint8_t> data = {0xFE, 0xDC, 0xBA, 0x98,
                                 0x76, 0x54, 0x32, 0x10};
    auto result = writer.writeBytes(targetAddr, data);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, static_cast<int>(data.size()));
    // Read back and compare
    {
        ProcMemIO memIO(childPid());
        ASSERT_TRUE(memIO.open(false));
        std::array<std::uint8_t, 8> buf{};
        auto readRes = memIO.read(targetAddr, std::span<std::uint8_t>(buf));
        ASSERT_TRUE(readRes.has_value());
        ASSERT_EQ(*readRes, buf.size());
        EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data.begin()))
            << "Byte array content mismatch";
    }
}

// Test: Write string
TEST_F(MemoryWriteTest, WriteString) {
    ASSERT_GT(childPid(), 0);
    // For string write test, reuse uint64 location as scratch area
    void* targetAddr = bufferAddr();
    ASSERT_NE(targetAddr, nullptr);

    MemoryWriter writer(childPid());
    const char* str = "Hello";
    auto result = writer.writeString(targetAddr, str);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, static_cast<int>(std::strlen(str) + 1))
        << "Should write string plus null terminator";
    // Read back and check prefix
    {
        ProcMemIO memIO(childPid());
        ASSERT_TRUE(memIO.open(false));
        std::array<char, 16> buf{};  // enough for test
        auto readRes = memIO.read(
            targetAddr,
            std::span<std::uint8_t>{reinterpret_cast<std::uint8_t*>(buf.data()),
                                    std::strlen(str) + 1});
        ASSERT_TRUE(readRes.has_value());
        ASSERT_EQ(*readRes, std::strlen(str) + 1);
        EXPECT_STREQ(buf.data(), str);
    }
}

// Test: Invalid PID
TEST(MemoryWriterTest, InvalidPid) {
    MemoryWriter writer(-1);

    int value = 42;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto result = writer.write(reinterpret_cast<void*>(0x1000), value);
    EXPECT_FALSE(result.has_value()) << "Write with invalid PID should fail";
}
