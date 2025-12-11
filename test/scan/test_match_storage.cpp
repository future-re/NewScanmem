// Unit tests for scan::match_storage
#include <gtest/gtest.h>

import scan.match_storage;
import value.flags;

using namespace scan;

TEST(MatchStorageTest, SwathAddElement) {
    MatchesAndOldValuesSwath swath;
    uint8_t testAddr = 0x42;
    void* addr = static_cast<void*>(&testAddr);

    swath.addElement(addr, 0xFF, MatchFlags::B8);

    EXPECT_EQ(swath.data.size(), 1);
    EXPECT_EQ(swath.data[0].oldByte, 0xFF);
    EXPECT_EQ(swath.data[0].matchInfo, MatchFlags::B8);
    EXPECT_EQ(swath.firstByteInChild, addr);
}

TEST(MatchStorageTest, SwathAppendRange) {
    MatchesAndOldValuesSwath swath;
    std::array<uint8_t, 4> buffer = {0x01, 0x02, 0x03, 0x04};
    void* addr = static_cast<void*>(buffer.data());

    swath.appendRange(addr, buffer.data(), buffer.size(), MatchFlags::B32);

    EXPECT_EQ(swath.data.size(), 4);
    EXPECT_EQ(swath.data[0].oldByte, 0x01);
    EXPECT_EQ(swath.data[3].oldByte, 0x04);
    EXPECT_EQ(swath.firstByteInChild, addr);
}

TEST(MatchStorageTest, SwathEmptyByDefault) {
    MatchesAndOldValuesSwath swath;

    EXPECT_EQ(swath.data.size(), 0);
    EXPECT_EQ(swath.firstByteInChild, nullptr);
}

TEST(MatchStorageTest, OldValueStructure) {
    OldValueAndMatchInfo info{};
    info.oldByte = 0xAB;
    info.matchInfo = MatchFlags::B16 | MatchFlags::B32;

    EXPECT_EQ(info.oldByte, 0xAB);
    EXPECT_NE(info.matchInfo, MatchFlags::EMPTY);
}

TEST(MatchStorageTest, AppendRangeDifferentFlagsAndBounds) {
    MatchesAndOldValuesSwath swath;
    std::array<uint8_t, 3> buffer = {0xAA, 0xBB, 0xCC};
    void* addr = static_cast<void*>(buffer.data());

    swath.appendRange(addr, buffer.data(), buffer.size(), MatchFlags::B8);
    ASSERT_EQ(swath.data.size(), 3);
    EXPECT_EQ(swath.data[0].oldByte, 0xAA);
    EXPECT_EQ(swath.data[1].oldByte, 0xBB);
    EXPECT_EQ(swath.data[2].oldByte, 0xCC);

    // Ensure match flags recorded
    EXPECT_EQ(swath.data[0].matchInfo, MatchFlags::B8);
    // Add single element after range
    uint8_t testVal = 0x11;
    void* addr2 = static_cast<void*>(&testVal);
    swath.addElement(addr2, testVal, MatchFlags::B64);
    EXPECT_EQ(swath.data.back().matchInfo, MatchFlags::B64);
}
