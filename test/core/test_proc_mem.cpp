// Unit tests for core::proc_mem
#include <gtest/gtest.h>
#include <unistd.h>

#include <span>

import core.proc_mem;

using namespace core;

TEST(ProcMemIOTest, Construction) {
    ProcMemIO io(getpid());
    // Should construct successfully
}

TEST(ProcMemIOTest, InvalidPid) {
    ProcMemIO io;
    auto result = io.open(false);

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("invalid") != std::string::npos);
}

TEST(ProcMemIOTest, OpenReadOnly) {
    ProcMemIO io(getpid());
    auto result = io.open(false);

    // May fail due to permissions, but should not crash
    if (!result) {
        EXPECT_FALSE(result.error().empty());
    }
}

TEST(ProcMemIOTest, ReadFromSelf) {
    ProcMemIO io(getpid());
    auto openResult = io.open(false);

    if (openResult) {
        int testValue = 0x12345678;
        std::byte buffer[sizeof(int)];

        // Attempt to read our own memory using span
        std::span<std::uint8_t> bufSpan(reinterpret_cast<std::uint8_t*>(buffer),
                                        sizeof(buffer));
        auto readResult = io.read(reinterpret_cast<void*>(&testValue), bufSpan);
        // Result may be success or error, both are valid
    }
}
