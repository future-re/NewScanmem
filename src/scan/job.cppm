/**
 * @file job.cppm
 * @brief Scan job setup with shared initialization logic
 *
 * Consolidates common setup code from engine and co_engine:
 * - Reading process maps
 * - Applying region filters
 * - Creating scan routines
 * - Opening /proc/<pid>/mem
 */

module;

#include <sys/types.h>

#include <expected>
#include <format>
#include <string>
#include <vector>

export module scan.job;

import scan.engine;  // For ScanOptions
import scan.types;
import scan.routine;
import scan.factory;
import core.maps;
import core.region_filter;
import core.proc_mem;
import value.flags;

export namespace scan {

/**
 * @class ScanJob
 * @brief Encapsulates scan initialization and shared resources
 *
 * Usage:
 *   ScanJob job(pid, opts);
 *   if (auto err = job.initialize(); !err) { return err; }
 *   auto& regions = job.getRegions();
 *   auto& routine = job.getRoutine();
 *   auto& memIO = job.getProcMemIO();
 */
class ScanJob {
   public:
    ScanJob(pid_t pid, const ScanOptions& opts)
        : m_pid(pid), m_opts(opts), m_memIO(pid) {}

    /**
     * @brief Initialize the scan job
     * @return Expected void or error message
     *
     * Performs all one-time setup:
     * 1. Read process memory maps
     * 2. Apply region filtering
     * 3. Create scan routine
     * 4. Open /proc/<pid>/mem
     */
    [[nodiscard]] auto initialize() -> std::expected<void, std::string> {
        // 1. Read process maps
        auto regionsExp = readProcessMaps(m_pid, m_opts.regionLevel);
        if (!regionsExp) {
            return std::unexpected{std::format("readProcessMaps failed: {}",
                                               regionsExp.error().message)};
        }
        m_regions = *regionsExp;

        // 2. Apply filtering
        if (m_opts.regionFilter.isScanTimeFilter() &&
            m_opts.regionFilter.filter.isActive()) {
            m_regions = m_opts.regionFilter.filter.filterRegions(m_regions);
        }

        // 3. Create scan routine
        m_routine = smGetScanroutine(
            m_opts.dataType, m_opts.matchType,
            MatchFlags::EMPTY,  // TODO: derive from user value
            m_opts.reverseEndianness);
        if (!m_routine) {
            return std::unexpected{"no scan routine for options"};
        }

        // 4. Open process memory
        if (auto err = m_memIO.open(); !err) {
            return std::unexpected{err.error()};
        }

        return {};
    }

    // Accessors
    [[nodiscard]] auto getRegions() const -> const std::vector<core::Region>& {
        return m_regions;
    }

    [[nodiscard]] auto getRoutine() const -> const scanRoutine& {
        return m_routine;
    }

    [[nodiscard]] auto getProcMemIO() -> core::ProcMemIO& {
        return m_memIO;
    }

    [[nodiscard]] auto getPid() const -> pid_t { return m_pid; }

    [[nodiscard]] auto getOptions() const -> const ScanOptions& {
        return m_opts;
    }

   private:
    pid_t m_pid;
    ScanOptions m_opts;
    std::vector<core::Region> m_regions;
    scanRoutine m_routine;
    core::ProcMemIO m_memIO;
};

}  // namespace scan
