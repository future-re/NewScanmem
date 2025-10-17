module;

#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <ranges>
#include <string>
#include <vector>

export module scan.engine;

import core.maps;      // readProcessMaps, Region, RegionScanLevel
import core.targetmem; // MatchesAndOldValuesArray, MatchesAndOldValuesSwath
import scan.factory;   // smGetScanroutine
import scan.types;     // ScanDataType / ScanMatchType
import value.flags;    // MatchFlags
import utils.mem64;    // Mem64
import value;          // Value / UserValue

// 轻量扫描引擎骨架：
// - 单线程、按块读取 /proc/<pid>/mem
// - 每字节位点调用 scanRoutine（步长可配置）
// - 结果存入 MatchesAndOldValuesArray（保存字节与标记）
// 限制：
// - 目前不跨块重叠匹配（跨边界匹配会遗漏）
// - 尚未接入“旧值快照”逻辑（oldValue 传 nullptr）
// - 仅实现只读扫描

export struct ScanOptions {
    ScanDataType dataType{ScanDataType::ANYNUMBER};
    ScanMatchType matchType{ScanMatchType::MATCHANY};
    bool reverseEndianness{false};
    std::size_t step{1};               // 扫描步长（按字节移动）
    std::size_t blockSize{64 * 1024};  // 每次读取的块大小
    RegionScanLevel regionLevel{RegionScanLevel::ALL};
};

export struct ScanStats {
    std::size_t regionsVisited{0};
    std::size_t bytesScanned{0};
    std::size_t matches{0};
};

// 简单的 /proc/<pid>/mem 读取器
export class ProcMemReader {
   public:
    ProcMemReader() = default;
    explicit ProcMemReader(pid_t pid) : m_pid(pid) {}

    [[nodiscard]] auto open() -> std::expected<void, std::string> {
        if (m_pid <= 0) {
            return std::unexpected{"invalid pid"};
        }
        std::string path = std::format("/proc/{}/mem", m_pid);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        int fileDesc = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fileDesc < 0) {
            return std::unexpected{
                std::format("open {} failed: {}", path, std::strerror(errno))};
        }
        m_fd = fileDesc;
        return {};
    }

    ~ProcMemReader() noexcept {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
    }

    ProcMemReader(const ProcMemReader&) = delete;
    auto operator=(const ProcMemReader&) -> ProcMemReader& = delete;
    ProcMemReader(ProcMemReader&& other) noexcept
        : m_pid(other.m_pid), m_fd(other.m_fd) {
        other.m_fd = -1;
    }
    auto operator=(ProcMemReader&& other) noexcept -> ProcMemReader& {
        if (this != &other) {
            if (m_fd >= 0) {
                ::close(m_fd);
            }
            m_pid = other.m_pid;
            m_fd = other.m_fd;
            other.m_fd = -1;
        }
        return *this;
    }

    // 尝试读取 [addr, addr+len) 到 buf；返回成功读取的字节数（可能小于 len）
    [[nodiscard]] auto read(void* addr, std::uint8_t* buf,
                            std::size_t len) const
        -> std::expected<std::size_t, std::string> {
        if (m_fd < 0) {
            return std::unexpected{"reader not opened"};
        }
        const auto OFFSET =
            static_cast<off_t>(std::bit_cast<std::uintptr_t>(addr));
        std::size_t total = 0;
        while (total < len) {
            const ssize_t RVAL = ::pread(m_fd, buf + total, len - total,
                                         static_cast<off_t>(OFFSET + total));
            if (RVAL < 0) {
                if (errno == EIO || errno == EFAULT || errno == EPERM ||
                    errno == EACCES) {
                    // 内核通常对不可读页返回错误；终止本块读取，返回已读部分
                    break;
                }
                return std::unexpected{
                    std::format("pread error: {}", std::strerror(errno))};
            }
            if (RVAL == 0) {
                break;  // 读到末尾
            }
            total += static_cast<std::size_t>(RVAL);
        }
        return total;
    }

   private:
    pid_t m_pid{-1};
    int m_fd{-1};
};

// 将读取的字节数据追加到 swath
inline void appendBytesToSwath(MatchesAndOldValuesSwath& swath,
                               const std::uint8_t* buffer,
                               std::size_t bytesRead, void* baseAddr) {
    if (swath.data.empty()) {
        swath.firstByteInChild = baseAddr;
    }

    auto toInsert =
        std::views::iota(static_cast<std::size_t>(0), bytesRead) |
        std::views::transform([&](std::size_t index) {
            return OldValueAndMatchInfo{.oldValue = buffer[index],
                                        .matchInfo = MatchFlags::EMPTY};
        });
    swath.data.insert(swath.data.end(), toInsert.begin(), toInsert.end());
}

// 对单个内存块执行扫描匹配
inline void scanBlock(const std::uint8_t* buffer, std::size_t bytesRead,
                      std::size_t baseIndex, std::size_t step, auto routine,
                      const UserValue* userValue,
                      MatchesAndOldValuesSwath& swath, ScanStats& stats) {
    for (std::size_t off = 0; off < bytesRead; off += step) {
        const std::size_t MEM_LEN = bytesRead - off;
        Mem64 mem{buffer + off, MEM_LEN};
        MatchFlags saveFlags = MatchFlags::EMPTY;
        const Value* oldValue = nullptr;  // TODO: 接入旧值快照

        const unsigned MATCHED_LEN =
            routine(&mem, MEM_LEN, oldValue, userValue, &saveFlags);

        if (MATCHED_LEN > 0) {
            swath.markRangeByIndex(baseIndex + off, MATCHED_LEN, saveFlags);
            stats.matches++;
        }
    }
}

// 处理单个内存区域的扫描
inline auto scanRegion(const Region& region, ProcMemReader& reader,
                       const ScanOptions& opts, auto routine,
                       const UserValue* userValue, ScanStats& stats)
    -> std::optional<MatchesAndOldValuesSwath> {
    if (!region.isReadable() || region.size == 0) {
        return std::nullopt;
    }

    stats.regionsVisited++;
    MatchesAndOldValuesSwath swath;
    std::vector<std::uint8_t> buffer(opts.blockSize);
    const std::size_t STEP = std::max<std::size_t>(1, opts.step);
    std::size_t regionOffset = 0;

    while (regionOffset < region.size) {
        const std::size_t REMAIN = region.size - regionOffset;
        const std::size_t TO_READ = std::min(REMAIN, opts.blockSize);
        auto* baseAddr =
            static_cast<std::uint8_t*>(region.start) + regionOffset;

        auto bytesReadExp = reader.read(baseAddr, buffer.data(), TO_READ);
        if (!bytesReadExp) {
            regionOffset += TO_READ;
            continue;
        }

        const std::size_t BYTES_READ = *bytesReadExp;
        if (BYTES_READ == 0) {
            regionOffset += TO_READ;
            continue;
        }

        const std::size_t BASE_INDEX = swath.data.size();
        appendBytesToSwath(swath, buffer.data(), BYTES_READ, baseAddr);
        scanBlock(buffer.data(), BYTES_READ, BASE_INDEX, STEP, routine,
                  userValue, swath, stats);

        stats.bytesScanned += BYTES_READ;
        regionOffset += BYTES_READ;
    }

    return swath.data.empty() ? std::nullopt : std::make_optional(swath);
}

export [[nodiscard]] inline auto runScan(pid_t pid, const ScanOptions& opts,
                                         const UserValue* userValue,
                                         MatchesAndOldValuesArray& out)
    -> std::expected<ScanStats, std::string> {
    ScanStats stats{};

    // 读取 maps
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) {
        return std::unexpected{std::format("readProcessMaps failed: {}",
                                           regionsExp.error().message)};
    }
    auto& regions = *regionsExp;

    // 准备匹配例程
    auto routine = smGetScanroutine(
        opts.dataType, opts.matchType,
        (userValue != nullptr) ? userValue->flags : MatchFlags::EMPTY,
        opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }

    // 打开 /proc/<pid>/mem
    ProcMemReader reader{pid};
    if (auto err = reader.open(); !err) {
        return std::unexpected{err.error()};
    }

    // 扫描所有区域
    for (const auto& region : regions) {
        if (auto swath =
                scanRegion(region, reader, opts, routine, userValue, stats)) {
            out.addSwath(*swath);
        }
    }

    return stats;
}
