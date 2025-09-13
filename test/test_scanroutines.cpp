// #include <gtest/gtest.h>

// #include <cstdint>

// import scanroutines;
// import value;

// // Helper to build Mem64 with a concrete value
// template <typename T>
// Mem64 makeMem64(T v) {
//     Mem64 m;  // Mem64 stores via variant; set via set<T>
//     m.set<T>(v);
//     return m;
// }

// class ScanRoutinesTest : public ::testing::Test {
//    protected:
//     UserValue user{};
//     Value old{};
//     MatchFlags flags = MatchFlags::EMPTY;
// };

// TEST_F(ScanRoutinesTest, EqualInt32) {
//     user.int32Value = 123;
//     Mem64 mem = makeMem64<int32_t>(123);
//     auto routine =
//         smGetScanroutine(ScanDataType::INTEGER32, ScanMatchType::MATCHEQUALTO,
//                          user.flags, false);
//     ASSERT_TRUE(static_cast<bool>(routine));
//     unsigned matched = routine(&mem, sizeof(int32_t), nullptr, &user, &flags);
//     EXPECT_EQ(matched, sizeof(int32_t));
//     EXPECT_NE(flags, MatchFlags::EMPTY);
// }

// TEST_F(ScanRoutinesTest, RangeInt32) {
//     user.int32Value = 100;           // low
//     user.int32RangeHighValue = 200;  // high
//     Mem64 mem = makeMem64<int32_t>(150);
//     auto routine = smGetScanroutine(
//         ScanDataType::INTEGER32, ScanMatchType::MATCHRANGE, user.flags, false);
//     ASSERT_TRUE(static_cast<bool>(routine));
//     unsigned matched = routine(&mem, sizeof(int32_t), nullptr, &user, &flags);
//     EXPECT_EQ(matched, sizeof(int32_t));
// }

// TEST_F(ScanRoutinesTest, RangeInt32ReversedBounds) {
//     user.int32Value = 200;           // low provided higher than high
//     user.int32RangeHighValue = 100;  // high smaller intentionally
//     Mem64 mem = makeMem64<int32_t>(150);
//     auto routine = smGetScanroutine(
//         ScanDataType::INTEGER32, ScanMatchType::MATCHRANGE, user.flags, false);
//     ASSERT_TRUE(static_cast<bool>(routine));
//     unsigned matched = routine(&mem, sizeof(int32_t), nullptr, &user, &flags);
//     EXPECT_EQ(matched, sizeof(int32_t));
// }

// TEST_F(ScanRoutinesTest, AnyIntegerMatchesUInt16) {
//     user.uint16Value = 42;  // for equality test
//     Mem64 mem = makeMem64<uint16_t>(42);
//     auto routine =
//         smGetScanroutine(ScanDataType::ANYINTEGER, ScanMatchType::MATCHEQUALTO,
//                          user.flags, false);
//     ASSERT_TRUE(static_cast<bool>(routine));
//     unsigned matched = routine(&mem, sizeof(uint16_t), nullptr, &user, &flags);
//     EXPECT_GT(matched, 0u);
// }

// TEST_F(ScanRoutinesTest, AnyNumberPrefersIntegerWhenBothPossible) {
//     // Store as int32_t
//     user.int32Value = 7;
//     Mem64 mem = makeMem64<int32_t>(7);
//     auto routine =
//         smGetScanroutine(ScanDataType::ANYNUMBER, ScanMatchType::MATCHEQUALTO,
//                          user.flags, false);
//     ASSERT_TRUE(static_cast<bool>(routine));
//     unsigned matched = routine(&mem, sizeof(int32_t), nullptr, &user, &flags);
//     EXPECT_EQ(matched, sizeof(int32_t));
// }

// TEST_F(ScanRoutinesTest, IncreasedBy) {
//     old.value = int32_t{100};
//     old.flags = MatchFlags::S32B;
//     user.int32Value = 10;  // delta
//     Mem64 mem = makeMem64<int32_t>(110);
//     auto routine =
//         smGetScanroutine(ScanDataType::INTEGER32,
//                          ScanMatchType::MATCHINCREASEDBY, user.flags, false);
//     ASSERT_TRUE(static_cast<bool>(routine));
//     unsigned matched = routine(&mem, sizeof(int32_t), &old, &user, &flags);
//     EXPECT_EQ(matched, sizeof(int32_t));
// }
