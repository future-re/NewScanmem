import core.maps;

#include <gtest/gtest.h>

#include <filesystem>
#include <ranges>
#include <sstream>

// 简单合成字符串测试
TEST(MapsParserTest, ParseSyntheticMaps) {
    const std::string SAMPLE = R"(
    00400000-0040c000 r-xp 00000000 08:02 123 /usr/bin/myprog
    0060b000-0060c000 r--p 0000b000 08:02 123 /usr/bin/myprog
    0060c000-0060d000 rw-p 0000c000 08:02 123 /usr/bin/myprog
    00e0c000-00e2d000 rw-p 00000000 00:00 0 [heap]
    7f7a3c000000-7f7a3c75d000 r-xp 00000000 08:02 654 /lib/x86_64-linux-gnu/libc-2.35.so
    )";
    std::istringstream iss(SAMPLE);
    auto regions = MapsReader::parseMapsFromStream(iss, "/usr/bin/myprog");

    // 期望解析到 5 个 region
    ASSERT_EQ(regions.size(), 5U);

    // 查找主程序 exe 的那一段（r-xp）
    auto exeRegionIt = std::ranges::find_if(
        regions, [](auto const &reg) { return reg.type == RegionType::EXE; });
    ASSERT_NE(exeRegionIt, std::ranges::end(regions));
    EXPECT_TRUE(exeRegionIt->isExecutable());
    EXPECT_EQ(exeRegionIt->filename, "/usr/bin/myprog");

    // 同一文件不同段，loadAddr 应该相同
    std::vector<Region> exeRegions;
    for (auto const &reg : regions) {
        if (reg.filename == "/usr/bin/myprog") {
            exeRegions.push_back(reg);
        }
    }
    ASSERT_GE(exeRegions.size(), 3U);
    for (auto const &reg : exeRegions) {
        EXPECT_EQ(reg.loadAddr, exeRegions.front().loadAddr);
    }

    // heap 应该被识别为 HEAP 且 loadAddr == start
    const auto HEAP_IT = std::ranges::find_if(
        regions, [](auto const &reg) { return reg.type == RegionType::HEAP; });
    ASSERT_TRUE(HEAP_IT != std::ranges::end(regions));
    EXPECT_EQ(HEAP_IT->loadAddr, HEAP_IT->start);
}

TEST(MapsParserTest, ParseProcSelfMaps) {
    // 读取 exe 路径
    const auto EXE_PATH =
        std::filesystem::read_symlink("/proc/self/exe").string();

    auto regions = MapsReader::readProcessMaps(getpid(), RegionScanLevel::ALL);
    ASSERT_TRUE(regions.has_value());
    // 至少有一个 exe/code 区域
    auto exeOrCodeIt =
        std::ranges::find_if(regions.value(), [&](auto const &reg) {
            return reg.filename == EXE_PATH && (reg.type == RegionType::EXE ||
                                                reg.type == RegionType::CODE);
        });
    ASSERT_NE(exeOrCodeIt, std::ranges::end(regions.value()));
    // 检查 main 地址属于某个 exe 域
    extern int main(int, char **);
    void *mainAddr = std::bit_cast<void *>(&main);
    // 在解析到的 regions 中找一个包含 mainAddr 的 exe 区域
    const auto CONST_ITER = std::ranges::find_if(
        regions->begin(), regions->end(), [&](auto const &reg) {
            return (reg.type == RegionType::EXE ||
                    reg.type == RegionType::CODE) &&
                   reg.contains(mainAddr);
        });
    EXPECT_NE(CONST_ITER, std::ranges::end(*regions));
}