// Unit tests for scan::match_storage
#include <gtest/gtest.h>

import scan.match_storage;
import value.flags;

using namespace scan;

TEST(MatchStorageTest, SwathAddElement) {
    MatchesAndOldValuesSwath swath;
    uint8_t testAddr = 0x42;
    void* addr = reinterpret_cast<void*>(&testAddr);

    swath.addElement(addr, 0xFF, MatchFlags::B8);

    EXPECT_EQ(swath.data.size(), 1);
    EXPECT_EQ(swath.data[0].oldByte, 0xFF);
    EXPECT_EQ(swath.data[0].matchInfo, MatchFlags::B8);
    EXPECT_EQ(swath.firstByteInChild, addr);
}

TEST(MatchStorageTest, SwathAppendRange) {
    MatchesAndOldValuesSwath swath;
    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    void* addr = reinterpret_cast<void*>(buffer);

    swath.appendRange(addr, buffer, 4, MatchFlags::B32);

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
    OldValueAndMatchInfo info;
    info.oldByte = 0xAB;
    info.matchInfo = MatchFlags::B16 | MatchFlags::B32;

    EXPECT_EQ(info.oldByte, 0xAB);
    EXPECT_NE(info.matchInfo, MatchFlags::EMPTY);
}
