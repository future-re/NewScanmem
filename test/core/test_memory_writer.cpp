import core.memory_writer;
import core.scanner;
import scan.match_storage;
import value.core;
import value.flags;

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

TEST(MemoryWriterMatchesTest, InvalidPidFails) {
    core::Scanner scanner(1234);
    core::MemoryWriter writer(-1);
    UserValue value = UserValue::fromScalar<int32_t>(100);

    auto result = writer.writeToMatch(scanner, value, {0});
    ASSERT_FALSE(result.has_value());
}
