import core.memory;
import core.maps;

#include <gtest/gtest.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <thread>

/**
 * @brief Test fixture for memory write operations
 *
 * Creates a child process with a known memory region that can be modified
 * from the parent process using MemoryWriter.
 */
class MemoryWriteTest : public ::testing::Test {
   protected:
    [[nodiscard]] auto childPid() const -> pid_t { return m_childPid; }
    [[nodiscard]] auto isChildReady() const -> bool {
        return m_childReady.load();
    }

    void SetUp() override {
        // Fork a child process for testing
        m_childPid = fork();

        if (m_childPid == 0) {
            // Child process: run target program
            runChildProcess();
            _exit(0);  // Child should never reach here
        } else if (m_childPid < 0) {
            FAIL() << "Failed to fork child process";
        }

        // Parent: wait for child to be ready
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
        // Test variables with known initial values
        volatile int testInt = 42;
        volatile float testFloat = 3.14F;
        volatile uint64_t testUint64 = 0x1234567890ABCDEF;

        // Keep process alive and prevent optimization
        while (true) {
            // Spin to keep values in memory
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Touch variables to prevent optimization
            if (testInt == -1 && testFloat < 0 && testUint64 == 0) {
                break;  // Exit condition (won't normally trigger)
            }
        }
    }

    /**
     * @brief Find address of a variable in child process
     * @return Address if found, nullptr otherwise
     */
    [[nodiscard]] auto findTestVariable(int expectedValue) const -> void* {
        auto mapsResult = MapsReader::readProcessMaps(m_childPid);
        if (!mapsResult) {
            return nullptr;
        }

        // Search writable regions for the test value
        // TODO: Implement memory scanning logic
        // For now, return nullptr as placeholder
        return nullptr;
    }

   private:
    pid_t m_childPid = -1;
    std::atomic<bool> m_childReady{false};
};

// Test: Write integer value
TEST_F(MemoryWriteTest, WriteIntValue) {
    ASSERT_GT(childPid(), 0) << "Child process should be created";

    // TODO: Find test variable address in child process
    // void* targetAddr = findTestVariable(42);
    // ASSERT_NE(targetAddr, nullptr) << "Should find test variable";

    // Create memory writer
    MemoryWriter writer(childPid());

    // TODO: Write new value
    // int newValue = 100;
    // auto result = writer.write(targetAddr, newValue);
    // ASSERT_TRUE(result.has_value()) << "Write should succeed";
    // EXPECT_EQ(*result, sizeof(int)) << "Should write 4 bytes";

    // TODO: Verify the write by reading back
}

// Test: Write float value
TEST_F(MemoryWriteTest, WriteFloatValue) {
    ASSERT_GT(childPid(), 0);

    MemoryWriter writer(childPid());

    // TODO: Implement float write test
}

// Test: Write byte array
TEST_F(MemoryWriteTest, WriteByteArray) {
    ASSERT_GT(childPid(), 0);

    MemoryWriter writer(childPid());

    // TODO: Implement byte array write test
    // Example:
    // std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    // auto result = writer.writeBytes(targetAddr, data);
}

// Test: Write string
TEST_F(MemoryWriteTest, WriteString) {
    ASSERT_GT(childPid(), 0);

    MemoryWriter writer(childPid());

    // TODO: Implement string write test
    // Example:
    // const char* str = "Hello";
    // auto result = writer.writeString(targetAddr, str);
}

// Test: Invalid PID
TEST(MemoryWriterTest, InvalidPid) {
    MemoryWriter writer(-1);

    int value = 42;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto result = writer.write(reinterpret_cast<void*>(0x1000), value);
    EXPECT_FALSE(result.has_value()) << "Write with invalid PID should fail";
}
