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
import scan.types; // ScanDataType / ScanMatchType / bytesNeededForType / matchUsesOldValue
import value.flags; // MatchFlags
import utils.mem64; // Mem64
import value;       // Value / UserValue

using core::MatchesAndOldValuesArray;
using core::MatchesAndOldValuesSwath;
using core::OldValueAndMatchInfo;
using core::Region;
using core::RegionScanLevel;

// Lightweight scan engine skeleton:
// - Single-threaded, reads /proc/<pid>/mem in blocks
// - Invokes a scanRoutine at each byte position (configurable step size)
// - Results are stored into MatchesAndOldValuesArray (records bytes and flags)
// Limitations:
// - Currently does not match across block boundaries (overlapping matches may
// be missed)
// - "oldValue" snapshot logic is not yet integrated (oldValue is passed as
// nullptr)
// - Read-only scanning is implemented

export struct ScanOptions {
    ScanDataType dataType{ScanDataType::ANYNUMBER};
    ScanMatchType matchType{ScanMatchType::MATCHANY};
    bool reverseEndianness{false};
    std::size_t step{1};               // scan step size (moves by bytes)
    std::size_t blockSize{64 * 1024};  // block size for each read
    RegionScanLevel regionLevel{RegionScanLevel::ALL_RW};
};

export struct ScanStats {
    std::size_t regionsVisited{0};
    std::size_t bytesScanned{0};
    std::size_t matches{0};
};

// /proc/<pid>/mem reader
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

    // Try to read [addr, addr+len) into buf; returns the number of bytes
    // successfully read (may be less than len).
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
                    // The kernel typically returns an error for unreadable
                    // pages; terminate reading this block and return the
                    // portion read.
                    break;
                }
                return std::unexpected{
                    std::format("pread error: {}", std::strerror(errno))};
            }
            if (RVAL == 0) {
                break;  // reached EOF
            }
            total += static_cast<std::size_t>(RVAL);
        }
        return total;
    }

   private: 
    pid_t m_pid{-1};
    int m_fd{-1};
};

// Append bytes read to the swath
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

// Execute scanning/matching for a single memory block
inline auto fetchOldBytes(const MatchesAndOldValuesArray& prev, void* addr,
                          std::size_t len, std::vector<std::uint8_t>& out)
    -> bool {
    if (addr == nullptr || len == 0) {
        return false;
    }
    for (const auto& swathPrev : prev.swaths) {
        if (swathPrev.firstByteInChild == nullptr || swathPrev.data.empty()) {
            continue;
        }
        const auto* base =
            static_cast<const std::uint8_t*>(swathPrev.firstByteInChild);
        const auto* curr = static_cast<const std::uint8_t*>(addr);
        if (curr < base) {
            continue;
        }
        const auto OFFSET = static_cast<std::size_t>(curr - base);
        if (OFFSET >= swathPrev.data.size()) {
            continue;
        }
        const std::size_t REMAIN = swathPrev.data.size() - OFFSET;
        if (REMAIN < len) {
            continue;
        }
        out.resize(len);
        for (std::size_t i = 0; i < len; ++i) {
            out[i] = swathPrev.data[OFFSET + i].oldValue;
        }
        return true;
    }
    return false;
}

inline void scanBlock(const std::uint8_t* buffer, std::size_t bytesRead,
                      std::size_t baseIndex, std::size_t step, auto routine,
                      const UserValue* userValue,
                      MatchesAndOldValuesSwath& swath, ScanStats& stats,
                      void* regionBlockBase,
                      const MatchesAndOldValuesArray* previousSnapshot,
                      bool usesOld, std::size_t oldSliceLen) {
    for (std::size_t off = 0; off < bytesRead; off += step) {
        const std::size_t MEM_LEN = bytesRead - off;
        Mem64 mem{buffer + off, MEM_LEN};
        MatchFlags saveFlags = MatchFlags::EMPTY;
        const Value* oldValue = nullptr;
        Value oldHolder;
        std::vector<std::uint8_t> oldBytes;
        if (usesOld && previousSnapshot != nullptr) {
            void* addr = static_cast<void*>(
                static_cast<std::uint8_t*>(regionBlockBase) + off);
            if (fetchOldBytes(*previousSnapshot, addr, oldSliceLen, oldBytes)) {
                oldHolder.setBytes(oldBytes);
                // Allow any scalar width for numeric extraction
                oldHolder.flags = MatchFlags::B8 | MatchFlags::B16 |
                                  MatchFlags::B32 | MatchFlags::B64;
                oldValue = &oldHolder;
            }
        }

        const unsigned MATCHED_LEN =
            routine(&mem, MEM_LEN, oldValue, userValue, &saveFlags);

        if (MATCHED_LEN > 0) {
            swath.markRangeByIndex(baseIndex + off, MATCHED_LEN, saveFlags);
            stats.matches++;
        }
    }
}

// Handle scanning for a single memory region
inline auto scanRegion(const Region& region, ProcMemReader& reader,
                       const ScanOptions& opts, auto routine,
                       const UserValue* userValue, ScanStats& stats,
                       const MatchesAndOldValuesArray* previousSnapshot,
                       bool usesOld, std::size_t oldSliceLen)
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
                  userValue, swath, stats, baseAddr, previousSnapshot, usesOld,
                  oldSliceLen);

        stats.bytesScanned += BYTES_READ;
        regionOffset += BYTES_READ;
    }

    return swath.data.empty() ? std::nullopt : std::make_optional(swath);
}

inline auto runScanInternal(pid_t pid, const ScanOptions& opts,
                            const UserValue* userValue,
                            MatchesAndOldValuesArray& out,
                            const MatchesAndOldValuesArray* previousSnapshot)
    -> std::expected<ScanStats, std::string> {
    ScanStats stats{};

    // Read process maps
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) {
        return std::unexpected{std::format("readProcessMaps failed: {}",
                                           regionsExp.error().message)};
    }
    auto& regions = *regionsExp;

    // Prepare scan routine
    auto routine = smGetScanroutine(
        opts.dataType, opts.matchType,
        (userValue != nullptr) ? userValue->flags : MatchFlags::EMPTY,
        opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }

    // Open /proc/<pid>/mem
    ProcMemReader reader{pid};
    if (auto err = reader.open(); !err) {
        return std::unexpected{err.error()};
    }

    const bool USES_OLD = matchUsesOldValue(opts.matchType);
    const std::size_t OLD_SLICE = bytesNeededForType(opts.dataType);

    // Scan all regions
    for (const auto& region : regions) {
        if (auto swath =
                scanRegion(region, reader, opts, routine, userValue, stats,
                           previousSnapshot, USES_OLD, OLD_SLICE)) {
            // If MATCH uses old value and we have previous snapshot, we will
            // still record bytes below; matching within scanBlock already
            // occurred with provided old value on a per-offset basis.
            out.addSwath(*swath);
        }
    }

    return stats;
}

// Backward compatible API: no previous snapshot
export [[nodiscard]] inline auto runScan(pid_t pid, const ScanOptions& opts,
                                         const UserValue* userValue,
                                         MatchesAndOldValuesArray& out)
    -> std::expected<ScanStats, std::string> {
    return runScanInternal(pid, opts, userValue, out, nullptr);
}

// New API: with previous snapshot for diff-based matches
export [[nodiscard]] inline auto runScanWithPrevious(
    pid_t pid, const ScanOptions& opts, const UserValue* userValue,
    MatchesAndOldValuesArray& out,
    const MatchesAndOldValuesArray& previousSnapshot)
    -> std::expected<ScanStats, std::string> {
    return runScanInternal(pid, opts, userValue, out, &previousSnapshot);
}
