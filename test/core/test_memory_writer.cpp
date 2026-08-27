#include "newscanmem/core/memory_writer.hpp"
#include "newscanmem/core/scanner.hpp"
#include "newscanmem/scan/match_storage.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

#include <gtest/gtest.h>
#include <unistd.h>

TEST(MemoryWriterMatchesTest, WritesToResolvedMatchAddress) {
    auto targetValue = int32_t{42};
    core::Scanner scanner(getpid());

    scan::MatchesAndOldValuesSwath swath;
    swath.firstByteInChild = &targetValue;
    swath.data.push_back(
        {.oldByte = 0x2A, .matchInfo = MatchFlags::B32, .matchLength = 4});
    scanner.getMatches().addSwath(swath);

    core::MemoryWriter writer(getpid());
    UserValue value = UserValue::fromScalar<int32_t>(100);

    auto result = writer.writeToMatch(scanner, value, {0});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->successCount, 1U);
    EXPECT_EQ(targetValue, 100);
}

TEST(MemoryWriterMatchesTest, WritesMultipleIndicesAndPreservesRequestOrder) {
    int32_t values[] = {10, 20, 30, 40};
    core::Scanner scanner(getpid());

    scan::MatchesAndOldValuesSwath swath;
    swath.firstByteInChild = values;
    swath.data.resize(sizeof(values));
    for (std::size_t offset = 0; offset < sizeof(values); offset += sizeof(int32_t)) {
        swath.data[offset].matchInfo = MatchFlags::B32;
        swath.data[offset].matchLength = sizeof(int32_t);
    }
    scanner.getMatches().addSwath(swath);

    core::MemoryWriter writer(getpid());
    UserValue value = UserValue::fromScalar<int32_t>(99);

    auto result = writer.writeToMatch(scanner, value, {3, 1});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->successCount, 2U);
    EXPECT_EQ(result->failedCount, 0U);
    EXPECT_EQ(values[0], 10);
    EXPECT_EQ(values[1], 99);
    EXPECT_EQ(values[2], 30);
    EXPECT_EQ(values[3], 99);
    ASSERT_EQ(result->results.size(), 2U);
    EXPECT_EQ(result->results[0].address,
              reinterpret_cast<std::uintptr_t>(&values[3]));
    EXPECT_EQ(result->results[1].address,
              reinterpret_cast<std::uintptr_t>(&values[1]));
}

TEST(MemoryWriterMatchesTest, InvalidPidFails) {
    core::Scanner scanner(1234);
    core::MemoryWriter writer(-1);
    UserValue value = UserValue::fromScalar<int32_t>(100);

    auto result = writer.writeToMatch(scanner, value, {0});
    ASSERT_FALSE(result.has_value());
}
