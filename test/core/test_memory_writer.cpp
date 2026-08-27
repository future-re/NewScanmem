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

TEST(MemoryWriterMatchesTest, InvalidPidFails) {
    core::Scanner scanner(1234);
    core::MemoryWriter writer(-1);
    UserValue value = UserValue::fromScalar<int32_t>(100);

    auto result = writer.writeToMatch(scanner, value, {0});
    ASSERT_FALSE(result.has_value());
}
