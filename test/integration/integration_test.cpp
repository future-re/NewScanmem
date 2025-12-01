// 说明：此文件目前不参与构建，仅作为示例占位。
// 实际的集成测试由同目录下的 CMakeLists.txt 通过 add_test 注册：
//  - Integration_CLI_Help: 运行可执行文件 `NewScanmem --help`
//  - Integration_Target_Runs: 运行 `targets/target_fixed_int` 并保证其正常退出
// 如需添加新的集成测试，可：
// 1) 在本目录新增可执行目标，或
// 2) 直接在 CMakeLists.txt 中用 `$<TARGET_FILE:...>` 注册命令型测试。

int main() { return 0; }