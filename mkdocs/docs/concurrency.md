# 并发架构设计与分析

本文档详细分析了 NewScanmem 在引入并发扫描时面临的挑战，并提出了基于 Map-Reduce 模型的解决方案。

## 1. 为什么目前的架构不支持并发？

目前的 `Scanner` 和 `Engine` 设计是单线程的。如果简单地尝试在 `runScanInternal` 中开启多线程（例如使用 OpenMP），程序将面临两个致命问题：**数据竞争 (Data Race)** 和 **资源竞争 (Resource Contention)**。

### A. 数据竞争 (Data Race)

这是最致命的问题。请看 `src/scan/engine.cppm` 中的核心扫描循环：

```cpp
// ❌ 伪代码：如果直接并行化
for (const auto& region : regions) {
    // 假设这里是多线程并行执行...
    if (auto swath = scanRegion(...)) {
        // 💥 崩溃点！
        // out 是一个 MatchesAndOldValuesArray，内部通常是 std::vector。
        // 当多个线程同时调用 push_back() 时：
        // 1. 可能会导致 vector 扩容（reallocate），旧指针失效，导致 Use-After-Free。
        // 2. 可能会导致 size 计数器错误，数据覆盖。
        out.addSwath(*swath); 
    }
}
```

**结论**：`out` (即 `MatchesAndOldValuesArray`) 是一个**共享的可变状态 (Shared Mutable State)**。在多线程环境下，必须对其加锁保护，或者消除共享。

### B. 资源竞争 (Resource Contention)

`ProcMemReader` 类持有一个指向 `/proc/pid/mem` 的文件描述符 (`m_fd`)。

*   **文件偏移量竞争**：如果底层使用 `lseek` + `read`，这是绝对线程不安全的。线程 A 刚 `lseek` 到位置 X，线程 B `lseek` 到位置 Y，紧接着线程 A 调用 `read` 就会读到 Y 的数据。
*   **内核锁竞争**：即使使用原子定位读 `pread`，在高并发下，多个线程争抢同一个内核文件对象锁，也会导致性能下降。

---

## 2. 解决方案：Map-Reduce 架构

为了实现高效且安全的并发，我们需要采用 **Map-Reduce (映射-归约)** 模式。

**核心思想**：让每个线程拥有自己的**私有结果容器**和**私有资源**，最后再合并。

### 架构演进

#### 第一步：Map 阶段 (并行)

将扫描任务（`regions` 列表）切分给多个工作线程。每个线程拥有：
1.  **私有结果容器** (`local_out`)：线程只向自己的容器写入数据，完全避免了对全局 `out` 的锁竞争。
2.  **私有 Reader** (`local_reader`)：每个线程打开自己的文件描述符，避免内核层面的锁竞争和文件偏移量混乱。

#### 第二步：Scan 阶段 (无锁)

扫描函数保持为**纯函数 (Pure Function)** 风格。它不依赖也不修改任何全局状态，只根据输入产生输出。

```cpp
// ✅ 线程安全的工作函数
auto worker = [&](std::span<Region> my_regions) -> MatchesAndOldValuesArray {
    MatchesAndOldValuesArray local_out; // ⭐️ 线程私有变量
    ProcMemReader local_reader(pid);    // ⭐️ 线程私有 Reader (避免 FD 竞争)
    
    for (const auto& region : my_regions) {
        if (auto swath = scanRegion(region, local_reader, ...)) {
            local_out.addSwath(*swath); // ✅ 安全：只写私有变量
        }
    }
    return local_out;
};
```

#### 第三步：Reduce 阶段 (合并)

当所有线程完成工作后，主线程负责将所有 `local_out` 合并到总的结果集中。

```cpp
MatchesAndOldValuesArray global_out;
for (auto& f : futures) {
    auto local_result = f.get();
    // 串行合并，或者使用细粒度锁并行合并
    global_out.merge(local_result); 
}

// (可选) 排序：并发扫描会导致结果乱序，通常需要按地址重新排序
global_out.sortByAddress();
```

## 3. 实施路线图

为了平滑过渡到并发架构，我们将分阶段进行：

1.  **Phase 1 (当前阶段)**：
    *   保持单线程执行。
    *   **关键目标**：确保 `scanRegion` 等核心函数保持**无状态 (Stateless)** 和**纯函数**特性。
    *   完善单元测试，确保逻辑正确性。

2.  **Phase 2 (未来规划)**：
    *   引入线程池 (`ThreadPool`) 或 `std::async`。
    *   实现 `MatchesAndOldValuesArray::merge()` 和 `sortByAddress()` 方法。
    *   重构 `runScanInternal` 为 Map-Reduce 模式。

通过这种设计，我们可以在不破坏现有功能的前提下，为未来支持大规模内存并发扫描打下坚实基础。
