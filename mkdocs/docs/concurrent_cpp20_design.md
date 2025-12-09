# C++20 并发扫描设计

本文档展示如何使用 C++20 新特性实现高效的并发内存扫描。

## 设计模式：Map-Reduce

### 核心思想

```
┌──────────────────────────────────────────────────────────┐
│  C++20 并发 Map-Reduce 模式                               │
└──────────────────────────────────────────────────────────┘

1. Map 阶段 (并行)
   ┌─────────┐   ┌─────────┐   ┌─────────┐
   │Thread 1 │   │Thread 2 │   │Thread 3 │
   │Region[0]│   │Region[3]│   │Region[6]│
   │Region[1]│   │Region[4]│   │Region[7]│
   │Region[2]│   │Region[5]│   │Region[8]│
   └────┬────┘   └────┬────┘   └────┬────┘
        │             │             │
        ↓             ↓             ↓
   local_out_1   local_out_2   local_out_3
   
2. Reduce 阶段 (串行/主线程)
   ┌──────────────────────────────┐
   │  Merge all local_out → out  │
   │  Sort by address (optional)  │
   └──────────────────────────────┘
```

## C++20 实现方案

### 方案 1: std::jthread + std::latch (推荐)

```cpp
#include <latch>
#include <thread>
#include <vector>
#include <span>

// 辅助结构：线程本地结果
struct LocalScanResult {
    MatchesAndOldValuesArray matches;
    ScanStats stats;
};

// 将 regions 分成 N 块
auto splitRegionsIntoChunks(std::span<const Region> regions, size_t numChunks) 
    -> std::vector<std::vector<Region>> 
{
    std::vector<std::vector<Region>> chunks(numChunks);
    
    // 策略1: 简单轮询
    for (size_t i = 0; i < regions.size(); ++i) {
        chunks[i % numChunks].push_back(regions[i]);
    }
    
    return chunks;
}

// 策略2: 按大小负载均衡
auto splitRegionsBySize(std::span<const Region> regions, size_t numChunks)
    -> std::vector<std::vector<Region>>
{
    if (regions.empty() || numChunks == 0) {
        return {};
    }
    
    // 计算总大小
    size_t totalSize = 0;
    for (const auto& r : regions) {
        totalSize += r.size;
    }
    
    size_t targetSizePerChunk = totalSize / numChunks;
    std::vector<std::vector<Region>> chunks;
    chunks.reserve(numChunks);
    
    std::vector<Region> currentChunk;
    size_t currentSize = 0;
    
    for (const auto& region : regions) {
        currentChunk.push_back(region);
        currentSize += region.size;
        
        // 达到目标大小，开始新的 chunk
        if (currentSize >= targetSizePerChunk && chunks.size() < numChunks - 1) {
            chunks.push_back(std::move(currentChunk));
            currentChunk.clear();
            currentSize = 0;
        }
    }
    
    // 添加最后一个 chunk
    if (!currentChunk.empty()) {
        chunks.push_back(std::move(currentChunk));
    }
    
    return chunks;
}

// ✅ C++20 并发扫描实现
auto runScanParallel(pid_t pid, const ScanOptions& opts,
                     const UserValue* userValue,
                     MatchesAndOldValuesArray& out,
                     const MatchesAndOldValuesArray* previousSnapshot)
    -> std::expected<ScanStats, std::string>
{
    // 1. 读取所有 Region（串行，快速）
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) {
        return std::unexpected{std::format("readProcessMaps failed: {}",
                                           regionsExp.error().message)};
    }
    auto regions = *regionsExp;
    
    // 2. 应用过滤（串行，快速）
    if (opts.regionFilter.isScanTimeFilter() &&
        opts.regionFilter.filter.isActive()) {
        regions = opts.regionFilter.filter.filterRegions(regions);
    }
    
    if (regions.empty()) {
        return ScanStats{};
    }
    
    // 3. 准备扫描例程（串行，快速）
    auto routine = smGetScanroutine(
        opts.dataType, opts.matchType,
        (userValue != nullptr) ? userValue->flags : MatchFlags::EMPTY,
        opts.reverseEndianness);
    if (!routine) {
        return std::unexpected{"no scan routine for options"};
    }
    
    const bool USES_OLD = matchUsesOldValue(opts.matchType);
    const std::size_t OLD_SLICE = bytesNeededForType(opts.dataType);
    
    // 4. 确定线程数量
    const size_t NUM_THREADS = std::min(
        std::thread::hardware_concurrency(),
        regions.size()  // 线程数不超过 region 数量
    );
    
    if (NUM_THREADS <= 1) {
        // 单线程回退到原始实现
        return runScanInternal(pid, opts, userValue, out, previousSnapshot);
    }
    
    // 5. 分配 Region 到各个线程
    auto chunks = splitRegionsBySize(regions, NUM_THREADS);
    
    // 6. 准备线程本地存储
    std::vector<LocalScanResult> results(chunks.size());
    std::latch workDone{static_cast<ptrdiff_t>(chunks.size())};  // C++20 同步原语
    
    // 7. ⭐ Map 阶段：启动并发扫描
    std::vector<std::jthread> threads;  // C++20 自动 join
    threads.reserve(chunks.size());
    
    for (size_t i = 0; i < chunks.size(); ++i) {
        threads.emplace_back([&, i, chunk = std::move(chunks[i])]() {
            // ⭐ 线程私有状态
            ProcMemReader local_reader{pid};
            if (auto err = local_reader.open(); !err) {
                workDone.count_down();
                return;
            }
            
            MatchesAndOldValuesArray& local_out = results[i].matches;
            ScanStats& local_stats = results[i].stats;
            
            // ⭐ 扫描分配给该线程的所有 Region
            for (const auto& region : chunk) {
                if (auto swath = scanRegion(region, local_reader, opts, routine,
                                           userValue, local_stats, previousSnapshot,
                                           USES_OLD, OLD_SLICE)) {
                    local_out.addSwath(*swath);  // 写入私有容器，无竞争
                }
            }
            
            workDone.count_down();  // 通知完成
        });
    }
    
    // 8. 等待所有线程完成
    workDone.wait();  // C++20 屏障，阻塞直到所有线程完成
    
    // 9. ⭐ Reduce 阶段：合并结果（串行，主线程）
    ScanStats total_stats{};
    for (auto& result : results) {
        // 合并匹配结果
        for (auto& swath : result.matches.swaths) {
            out.addSwath(swath);
        }
        
        // 累加统计信息
        total_stats.regionsVisited += result.stats.regionsVisited;
        total_stats.bytesScanned += result.stats.bytesScanned;
        total_stats.matches += result.stats.matches;
    }
    
    // 10. (可选) 按地址排序
    // out.sortByAddress();  // 待实现
    
    return total_stats;
}
```

### 方案 2: std::async + std::future (简单但效率稍低)

```cpp
#include <future>
#include <vector>

auto runScanParallelAsync(pid_t pid, const ScanOptions& opts,
                          const UserValue* userValue,
                          MatchesAndOldValuesArray& out,
                          const MatchesAndOldValuesArray* previousSnapshot)
    -> std::expected<ScanStats, std::string>
{
    // 前期准备同上...
    auto regionsExp = readProcessMaps(pid, opts.regionLevel);
    if (!regionsExp) return std::unexpected{...};
    auto regions = *regionsExp;
    
    // 应用过滤...
    
    auto routine = smGetScanroutine(...);
    if (!routine) return std::unexpected{...};
    
    const bool USES_OLD = matchUsesOldValue(opts.matchType);
    const std::size_t OLD_SLICE = bytesNeededForType(opts.dataType);
    
    // 分配 Region
    const size_t NUM_THREADS = std::min(
        std::thread::hardware_concurrency(),
        regions.size()
    );
    auto chunks = splitRegionsBySize(regions, NUM_THREADS);
    
    // ⭐ 使用 std::async 启动异步任务
    std::vector<std::future<LocalScanResult>> futures;
    futures.reserve(chunks.size());
    
    for (auto& chunk : chunks) {
        futures.push_back(std::async(std::launch::async, 
            [&, chunk = std::move(chunk)]() -> LocalScanResult {
                // 线程私有状态
                ProcMemReader local_reader{pid};
                if (auto err = local_reader.open(); !err) {
                    return {};  // 返回空结果
                }
                
                LocalScanResult result;
                for (const auto& region : chunk) {
                    if (auto swath = scanRegion(region, local_reader, opts, routine,
                                               userValue, result.stats, previousSnapshot,
                                               USES_OLD, OLD_SLICE)) {
                        result.matches.addSwath(*swath);
                    }
                }
                return result;
            }
        ));
    }
    
    // ⭐ Reduce: 等待并合并结果
    ScanStats total_stats{};
    for (auto& f : futures) {
        auto result = f.get();  // 阻塞等待
        
        for (auto& swath : result.matches.swaths) {
            out.addSwath(swath);
        }
        
        total_stats.regionsVisited += result.stats.regionsVisited;
        total_stats.bytesScanned += result.stats.bytesScanned;
        total_stats.matches += result.stats.matches;
    }
    
    return total_stats;
}
```

### 方案 3: C++23 std::execution (未来标准)

```cpp
// C++23 标准库并行算法 (需要编译器支持)
#include <execution>
#include <algorithm>

auto runScanParallelExecution(/* ... */) {
    // 使用标准库并行算法
    std::for_each(std::execution::par,  // 并行执行策略
                  regions.begin(), regions.end(),
                  [&](const Region& region) {
                      // 问题：需要线程本地存储，标准库不直接支持
                      // 实际应用中可能需要 thread_local 或 TLS
                  });
}
```

## 关键对比

| 特性 | std::jthread + latch | std::async + future | 传统 thread |
|------|---------------------|-----------------------|-------------|
| **RAII** | ✅ 自动 join | ✅ 自动销毁 | ❌ 手动 join |
| **同步** | ✅ latch/barrier | ⚠️ future.get() | ❌ 手动锁 |
| **取消** | ✅ stop_token | ❌ 不支持 | ❌ 不支持 |
| **代码简洁** | ✅ 很好 | ✅ 最好 | ⚠️ 冗长 |
| **性能** | ✅ 最好 | ⚠️ 稍有开销 | ✅ 很好 |
| **C++ 版本** | C++20 | C++11 | C++11 |

## 推荐方案

### 生产环境：std::jthread + std::latch
- ✅ 性能最优
- ✅ RAII 保证安全
- ✅ 支持协作式取消（stop_token）
- ✅ 代码清晰

### 原型开发：std::async
- ✅ 最简单
- ✅ 适合快速验证
- ⚠️ 线程池开销

## 需要实现的辅助函数

```cpp
// 1. MatchesAndOldValuesArray::merge() - 合并两个结果集
void MatchesAndOldValuesArray::merge(const MatchesAndOldValuesArray& other) {
    swaths.insert(swaths.end(), other.swaths.begin(), other.swaths.end());
}

// 2. MatchesAndOldValuesArray::sortByAddress() - 按地址排序
void MatchesAndOldValuesArray::sortByAddress() {
    std::ranges::sort(swaths, [](const auto& a, const auto& b) {
        return a.firstByteInChild < b.firstByteInChild;
    });
}

// 3. ScanStats::operator+= - 累加统计
ScanStats& operator+=(ScanStats& lhs, const ScanStats& rhs) {
    lhs.regionsVisited += rhs.regionsVisited;
    lhs.bytesScanned += rhs.bytesScanned;
    lhs.matches += rhs.matches;
    return lhs;
}
```

## 总结

C++20 的并发特性完美契合 Map-Reduce 模式：

1. **Map 阶段**：`std::jthread` 自动管理线程生命周期
2. **同步**：`std::latch` 提供轻量级屏障
3. **Reduce 阶段**：主线程串行合并，无需锁
4. **安全性**：RAII 保证资源清理

这种设计模式保持不变，只是用 C++20 的新特性让代码更简洁、更安全！
