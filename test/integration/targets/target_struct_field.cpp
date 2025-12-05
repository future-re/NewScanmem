#include <cstdint>

#include "modification_framework.h"

// 测试目标：结构体字段
// 用于测试扫描和修改结构体中的特定字段
namespace {
struct Player {
    std::int32_t health;
    std::int32_t mana;
    std::int32_t level;
    std::int32_t gold;
};
}  // namespace

auto main(int argc, char** argv) -> int {
    // 定义一个玩家数据结构
    static volatile Player player = {
        .health = 100,  // health
        .mana = 50,     // mana
        .level = 10,    // level
        .gold = 9999    // gold
    };

    // 监控 gold 字段
    constexpr std::int32_t EXPECTED_VALUE = 9999;
    constexpr std::int32_t MODIFIED_VALUE = 1000000;

    // 打印结构体内容
    std::cout << "Player data:" << std::endl;
    std::cout << "  Health: " << player.health << std::endl;
    std::cout << "  Mana: " << player.mana << std::endl;
    std::cout << "  Level: " << player.level << std::endl;
    std::cout << "  Gold: " << player.gold << std::endl;
    std::cout << "Monitoring player gold for modification..." << std::endl;

    // 使用通用框架运行测试（监控结构体中的特定字段）
    return modification_framework::runModificationTest(
        argc, argv, player.gold, EXPECTED_VALUE, MODIFIED_VALUE);
}
