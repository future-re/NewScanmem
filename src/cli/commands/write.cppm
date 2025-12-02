/**
 * @file write.cppm
 * @brief Write command: modify memory at matched addresses
 */

module;
 
#include <bit>
#include <charconv>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.write;

import cli.command;
import cli.session;
import ui.show_message;
import core.scanner;
import core.memory_writer;
import value.flags;
import value.parser;

export namespace cli::commands {

class WriteCommand : public Command {
   public:
    explicit WriteCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "write";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Write value to memory at matched addresses";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "write <value> [index]\n"
               "  value: 要写入的值 (支持十六进制 0x...)\n"
               "  index (可选): 匹配索引 (默认: 写入所有匹配)\n"
               "  示例: write 100 / write 0xff 0";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (args.empty()) {
            return std::unexpected("Usage: write <value> [index]");
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || m_session->pid <= 0) {
            return std::unexpected("Set target pid first: pid <pid>");
        }
        if (!m_session->scanner) {
            return std::unexpected("No matches. Run a scan first.");
        }

        // 解析值（统一为 64 位，按匹配宽度截断写入）
        std::uint64_t value = 0;
        const auto& valueStr = args[0];
        auto valueOpt = value::parseInt64(valueStr);
        if (!valueOpt) {
            return std::unexpected("Invalid value: " + valueStr);
        }
        value = static_cast<std::uint64_t>(*valueOpt);

        // 解析索引（可选）
        std::optional<size_t> targetIndex;
        if (args.size() > 1) {
            try {
                targetIndex = std::stoull(args[1]);
            } catch (...) {
                return std::unexpected("Invalid index: " + args[1]);
            }
        }

        // 创建写入器
        core::MemoryWriter writer(m_session->pid);

        auto* scanner = m_session->scanner.get();
        const auto& matches = scanner->getMatches();

        size_t currentIndex = 0;
        size_t writeCount = 0;

        for (const auto& swath : matches.swaths) {
            auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
            if (base == nullptr) {
                continue;
            }

            for (std::size_t i = 0; i < swath.data.size(); ++i) {
                const auto& cell = swath.data[i];
                if (cell.matchInfo == MatchFlags::EMPTY) {
                    continue;
                }

                if (targetIndex && currentIndex != *targetIndex) {
                    currentIndex++;
                    continue;
                }

                // 计算匹配宽度
                auto flags = swath.data[i].matchInfo;
                std::size_t width = 1;
                if ((flags & MatchFlags::B64) == MatchFlags::B64) {
                    width = 8;
                } else if ((flags & MatchFlags::B32) == MatchFlags::B32) {
                    width = 4;
                } else if ((flags & MatchFlags::B16) == MatchFlags::B16) {
                    width = 2;
                } else {
                    width = 1;
                }

                std::uintptr_t addr = 0;
                std::size_t writeLen = 1;

                if (targetIndex) {
                    // 回溯到段起始
                    std::size_t s = i;
                    while (s > 0) {
                        const auto prevFlags = swath.data[s - 1].matchInfo;
                        if ((prevFlags & flags) != flags) {
                            break;
                        }
                        --s;
                    }
                    addr = std::bit_cast<std::uintptr_t>(base + s);
                    writeLen = width;
                } else {
                    addr = std::bit_cast<std::uintptr_t>(base + i);
                    writeLen = 1;
                }

                // 使用 MemoryWriter 写入
                auto writeRes = writer.write(addr, value, writeLen);

                if (writeRes) {
                    writeCount++;
                    if (writeLen == 1) {
                        ui::MessagePrinter::info(std::format(
                            "Written 0x{:02x} to 0x{:016x}",
                            static_cast<unsigned>(value & 0xFF), addr));
                    } else {
                        ui::MessagePrinter::info(
                            std::format("Written {}B value 0x{:x} to 0x{:016x}",
                                        writeLen, value, addr));
                    }
                } else {
                    ui::MessagePrinter::warn(
                        std::format("Failed to write ({}B) to 0x{:016x}: {}",
                                    writeLen, addr, writeRes.error()));
                }

                if (targetIndex) {
                    break;
                }

                currentIndex++;
            }

            if (targetIndex && writeCount > 0) {
                break;
            }
        }

        if (writeCount == 0) {
            return std::unexpected("No values written");
        }

        ui::MessagePrinter::success(
            std::format("Successfully wrote {} value(s)", writeCount));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
