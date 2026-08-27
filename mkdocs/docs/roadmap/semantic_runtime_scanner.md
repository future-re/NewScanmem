# NewScanmem 下一代特性规划：Semantic Runtime Scanner

## 1. 背景与目标

NewScanmem 当前定位是现代 C++ 实现的进程内存扫描工具，核心能力仍围绕数值、字符串、字节模式扫描以及基于上一次扫描结果的过滤展开。

传统内存扫描器通常回答的是：

- 某个数值存在哪个地址？
- 哪些地址发生了增加、减少或变化？
- 如何读取或修改目标进程中的某段内存？

下一阶段希望将 NewScanmem 从 **Memory Scanner** 演进为 **Semantic Runtime Scanner**：不仅发现“哪个地址匹配”，还能够描述“程序运行时发生了什么”。

核心演进方向：

```text
byte / value / address
        ↓
object / state / relationship
        ↓
behavior / timeline / semantic finding
```

最终目标是形成如下处理模型：

```text
observe
  ↓
snapshot
  ↓
diff
  ↓
detect
  ↓
finding
  ↓
explain
```

---

## 2. Memory Timeline

Memory Timeline 用于持续记录目标进程内存状态随时间的变化，而不是只保留一次扫描及其结果。

计划能力：

- 周期性采集内存快照；
- 记录内存区域的创建、增长、变化和消失；
- 查询某个地址或区域在过去一段时间内的变化历史；
- 支持按时间窗口分析，例如“过去 10 秒变化最频繁的区域”；
- 为后续对象生命周期、Secret 生命周期和 Runtime Event 提供统一时间轴。

示例：

```text
12:31:01  region 0x7f120000 created   8 MB
12:31:03  region 0x7f120000 changed   63%
12:31:05  region 0x7f120000 entropy   2.1 -> 7.8
12:31:11  region 0x7f120000 released
```

建议引入核心概念：

```cpp
struct MemorySnapshot;
struct MemoryEvent;
struct MemoryTimeline;
```

---

## 3. Semantic Diff

现有扫描通常只关心 equal、changed、increased、decreased 等值级比较。Semantic Diff 将比较范围扩展到内存区域和运行时状态。

候选检测项：

- 新出现的内存区域；
- 消失的区域；
- 区域大小变化；
- 高频修改区域；
- 字符串出现或消失；
- 内存熵显著变化；
- 指针关系变化；
- 疑似结构体布局变化；
- 大块内存突增或突降。

输出不再只是地址列表，而是结构化 Finding：

```cpp
struct Finding {
    Address address;
    FindingType type;
    float confidence;
    std::string description;
};
```

示例：

```text
Finding: HighEntropyRegion
Address: 0x7f120000
Size: 4 MB
Entropy: 2.4 -> 7.9
Confidence: 0.93
```

---

## 4. Memory Hotspot

Memory Hotspot 用于发现“哪些内存位置最活跃”。它类似 CPU profiler，但关注内存修改频率和变化强度。

计划能力：

- 统计地址或区域的修改次数；
- 计算 writes/s 或 change-rate；
- 对热点地址进行聚类；
- 与线程、调用栈或时间线关联；
- 找出异常高频更新的数据结构。

示例：

```text
0x7f100010   39123 changes/s
0x7f100080    8211 changes/s
0x7f102000    2190 changes/s
```

该能力可用于实时程序、游戏、服务进程和 Agent Runtime 的状态分析。

---

## 5. Pointer / Object Graph

传统 pointer scan 主要用于找到稳定指针链。下一阶段可以进一步构建运行时对象引用图。

核心思路：

```text
Memory Address / Object
          │
        pointer
          ↓
Memory Address / Object
```

计划能力：

- 自动识别疑似指针；
- 判断目标地址所属内存区域；
- 构建对象引用关系图；
- 查询谁引用了某个对象；
- 查询对象是否可以从全局/栈 root 到达；
- 检测循环引用或异常引用链；
- 将引用图与 Memory Timeline 联动。

这使 NewScanmem 可以从“地址扫描器”逐步进入 Heap/Object Inspector 的能力范围。

---

## 6. Automatic Structure Recovery

Structure Recovery 用于在缺少源码和调试符号时，对重复内存布局进行推断。

可以综合以下特征：

- 地址对齐；
- 整数/浮点数分布；
- 有效指针；
- 指针指向字符串；
- 重复布局；
- 字段随时间的变化规律；
- 不同对象实例之间的相似性。

示例：

```text
Candidate Object @ 0x1000

+0x00  int32   1234
+0x08  char*   "alice"
+0x10  float   98.5
+0x18  ptr     CandidateObject
```

该功能不要求一次性恢复完整 C++ 类型，而是生成带置信度的候选布局。

---

## 7. Memory Region Classification

在现有 `/proc/<pid>/maps` 和内存读取能力基础上，对区域增加更高层分类。

基础类别：

- heap；
- stack；
- executable mapping；
- file-backed mmap；
- anonymous mapping；
- shared memory。

进一步可识别：

- 疑似对象池；
- 疑似压缩数据；
- 疑似加密数据；
- 大型缓存；
- 高频更新 buffer；
- 长生命周期只读数据。

区域分类结果可作为其他 detector 的输入，而不是单纯作为 UI 标签。

---

## 8. Secret Scanner

增加面向安全分析的敏感信息检测能力。

候选类型：

- API Key；
- JWT；
- Bearer Token；
- Access Token；
- Private Key；
- Password-like string；
- Credential；
- 其他可配置敏感数据模式。

检测不应只依赖正则表达式，可以综合：

```text
pattern
  + entropy
  + region type
  + lifetime
  + copy relationship
  + reference relationship
```

示例：

```text
Potential Secret

Type: Bearer Token
Address: 0x7f223000
Region: heap
First Seen: 12:32:08
Lifetime: 12.3 s
Copies: 3
```

后续可进一步研究敏感数据从创建、复制到释放的 Runtime Data Flow。

---

## 9. Allocation Tracking

从“地址存在什么”进一步升级到“这块内存从哪里来、活了多久”。

计划能力：

- 记录 allocation/free；
- 记录 allocation size；
- 记录生命周期；
- 关联线程；
- 可选关联调用栈；
- 检测长期未释放对象；
- 识别频繁分配/释放热点。

示例：

```text
Allocation 0x12340000
Size: 4 MB
Created: 12:30:01
Last Changed: 12:30:09
Freed: 12:30:12
Lifetime: 11 s
```

实现层可逐步评估 malloc hook、uprobes、eBPF 等机制，并与现有扫描引擎保持解耦。

---

## 10. Cross-layer Runtime View

长期目标之一是将内存事件和其他运行时事件关联起来。

```text
process
   ↓
thread
   ↓
syscall / function / allocation
   ↓
memory region
   ↓
semantic finding
```

典型问题：

- 哪个线程导致了这块区域发生变化？
- 哪次 allocation 创建了这个对象？
- 哪次 read/socket 操作后出现了这段高熵 buffer？
- 某个敏感字符串何时进入进程内存？

这一层可以让 NewScanmem 从内存分析工具进一步演进为 Runtime Inspector。

---

## 11. AI / Natural Language Scan

AI 不应直接扫描数 GB 原始内存，而应该负责“生成扫描计划”和“解释结构化结果”。

推荐模型：

```text
Natural Language Intent
          ↓
      Scan Plan
          ↓
C++ Scanner / Detector Pipeline
          ↓
       Findings
          ↓
 Semantic Explanation
```

示例：

```text
用户：找最近 5 秒新出现的大块高熵内存。
```

可转换为：

```text
window = 5s
region_state = new
size > 1 MB
entropy > 7.0
```

或者：

```text
用户：找看起来像用户对象的数据结构。
```

由 AI 组合：

```text
RepeatedLayoutDetector
+ PointerDetector
+ StringReferenceDetector
+ TemporalCorrelationDetector
```

AI 层的重点是规划、组合和解释，而不是替代底层扫描器。

---

## 12. AI Runtime / Agent Runtime 扩展方向

在基础 Runtime Scanner 成熟后，可以继续增加面向 AI Runtime 的专用 Detector。

候选方向：

- Tensor buffer 识别；
- Model mmap 识别；
- KV Cache 变化追踪；
- CPU/GPU 内存关联；
- Token/Request 与内存增长关联；
- Agent tool 调用前后的 Runtime Diff；
- Agent 产生的文件、进程、网络和内存行为关联。

这部分应建立在通用 Timeline、Finding 和 Detector 架构之上，而不应该和核心内存扫描逻辑强耦合。

---

## 13. Scanner 架构演进

建议逐步将扫描模块从按“数据类型”组织，扩展为按“检测语义”组织：

```text
scanner/
    memory/
        numeric
        string
        bytes

    structure/
        pointer
        object
        graph

    temporal/
        snapshot
        diff
        mutation

    security/
        secret
        credential

    runtime/
        allocation
        mmap
        hotspot

    ai/
        tensor
        kv_cache
```

统一抽象可以逐步收敛为：

```text
ScanRequest
    ↓
Scanner / Detector
    ↓
Finding
```

这样现有 numeric/string/bytes scanner 可以继续作为基础能力存在，而不需要推翻重写。

---

## 14. 推荐实施顺序

### Phase 1：Temporal Runtime

优先实现：

1. `MemorySnapshot`；
2. `MemoryTimeline`；
3. `MemoryEvent`；
4. Semantic Diff；
5. Memory Hotspot。

目标：从“一次扫描”升级为“持续观察”。

### Phase 2：Semantic Memory

实现：

1. Pointer Detector；
2. Object Graph；
3. Structure Recovery；
4. Region Classification；
5. Secret Scanner。

目标：从“地址和值”升级为“对象、关系和语义”。

### Phase 3：Runtime Inspector

实现：

1. Allocation Tracking；
2. Runtime Event Correlation；
3. Cross-layer Runtime View；
4. 可插拔 Detector Pipeline。

目标：解释程序运行过程中发生了什么。

### Phase 4：AI-assisted Scanner

实现：

1. Natural Language Scan；
2. Scan Plan 生成；
3. Findings 自动解释；
4. AI Runtime / Agent Runtime 专用 Detector。

目标：让用户从“指定地址和值”转向“描述问题和意图”。

---

## 15. 项目定位演进

当前定位：

> Modern C++ memory scanner.

推荐长期定位：

> **NewScanmem — Semantic Runtime Scanner**
>
> Inspect not only what exists in memory, but how runtime state changes, relates and behaves.

这一路线的关键不是给传统 scanmem 加一个聊天框，而是让扫描对象本身从 `value/address` 升级为 `state/object/relationship/behavior/timeline`。
