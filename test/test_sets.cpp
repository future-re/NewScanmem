#include <gtest/gtest.h>

#include <vector>

import sets;

TEST(ParseUintSetTest, ValidInput) {
    Set set;
    EXPECT_TRUE(parse_uintset("1,2,3", set, 10));
    EXPECT_EQ(set.size(), 3);
    EXPECT_EQ(set.buf, std::vector<size_t>({1, 2, 3}));

    EXPECT_TRUE(parse_uintset("0x1,0x2,0x3", set, 10));
    EXPECT_EQ(set.size(), 3);
    EXPECT_EQ(set.buf, std::vector<size_t>({1, 2, 3}));

    EXPECT_TRUE(parse_uintset("1..3", set, 10));
    EXPECT_EQ(set.size(), 3);
    EXPECT_EQ(set.buf, std::vector<size_t>({1, 2, 3}));
}

TEST(ParseUintSetTest, InvertedInput) {
    Set set;
    EXPECT_TRUE(parse_uintset("!1,2,3", set, 5));
    EXPECT_EQ(set.size(), 2);
    EXPECT_EQ(set.buf, std::vector<size_t>({0, 4}));
}

TEST(ParseUintSetTest, InvalidInput) {
    Set set;
    EXPECT_FALSE(parse_uintset("1..10", set, 5));  // 超出范围
    EXPECT_FALSE(parse_uintset("abc", set, 10));   // 非法字符
    EXPECT_FALSE(parse_uintset("1..", set, 10));   // 不完整范围
}

TEST(ParseUintSetTest, EmptyInput) {
    Set set;
    EXPECT_FALSE(parse_uintset("", set, 10));  // 空输入
}

TEST(ParseUintSetTest, EdgeCases) {
    Set set;
    EXPECT_TRUE(parse_uintset("0", set, 1));
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.buf, std::vector<size_t>({0}));

    EXPECT_FALSE(parse_uintset("!0", set, 1));
}