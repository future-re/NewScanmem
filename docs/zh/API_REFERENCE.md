# API 参考

## 模块索引

- [endianness](#endianness)
- [maps](#maps)  
- [process_checker](#process_checker)
- [sets](#sets)
- [show_message](#show_message)
- [targetmem](#targetmem)
- [value](#value)

---

## maps

### 枚举：`region_type`

```cpp
enum class region_type : uint8_t {
    misc,   // 杂项内存区域
    exe,    // 可执行文件二进制区域
    code,   // 代码段（共享库等）
    heap,   // 堆内存区域
    stack   // 栈内存区域
};

constexpr std::array<std::string_view, 5> region_type_names;
```

### 枚举：`region_scan_level`

```cpp
enum class region_scan_level : uint8_t {
    all,                       // 所有可读区域
    all_rw,                    // 所有可读/可写区域
    heap_stack_executable,     // 堆、栈和可执行区域
    heap_stack_executable_bss  // 上述加上 BSS 段
};
```

### 结构：`region_flags`

```cpp
struct region_flags {
    bool read : 1;    // 读权限
    bool write : 1;   // 写权限
    bool exec : 1;    // 执行权限
    bool shared : 1;  // 共享映射
    bool private_ : 1; // 私有映射
};
```

### 结构：`region`

```cpp
struct region {
    void* start;           // 起始地址
    std::size_t size;      // 区域大小（字节）
    region_type type;      // 区域分类
    region_flags flags;    // 权限标志
    void* load_addr;       // ELF 文件的加载地址
    std::string filename;  // 关联文件路径
    std::size_t id;        // 唯一标识符

    [[nodiscard]] bool is_readable() const noexcept;
    [[nodiscard]] bool is_writable() const noexcept;
    [[nodiscard]] bool is_executable() const noexcept;
    [[nodiscard]] bool is_shared() const noexcept;
    [[nodiscard]] bool is_private() const noexcept;
    [[nodiscard]] std::pair<void*, std::size_t> as_span() const noexcept;
    [[nodiscard]] bool contains(void* address) const noexcept;
};
```

### 结构：`maps_reader::error`

```cpp
struct error {
    std::string message;   // 人类可读的错误描述
    std::error_code code;  // 系统错误代码
};
```

### 类：`maps_reader`

#### 静态方法

```cpp
[[nodiscard]] static std::expected<std::vector<region>, error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all);
```

### 便利函数

```cpp
[[nodiscard]] std::expected<std::vector<region>, maps_reader::error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all);
```

#### 使用示例

```cpp
import maps;

// 基本用法
auto regions = maps::read_process_maps(1234);

// 过滤扫描
auto heap_regions = maps::read_process_maps(
    pid, 
    maps::region_scan_level::heap_stack_executable
);

// 错误处理
if (!regions) {
    std::cerr << "错误: " << regions.error().message << "\n";
}
```

---

## endianness

### 命名空间: `endianness`

#### 函数

```cpp
// 字节序检测
constexpr bool is_big_endian() noexcept;
constexpr bool is_little_endian() noexcept;

// 字节交换
constexpr uint8_t swap_bytes(uint8_t value) noexcept;
constexpr uint16_t swap_bytes(uint16_t value) noexcept;
constexpr uint32_t swap_bytes(uint32_t value) noexcept;
constexpr uint64_t swap_bytes(uint64_t value) noexcept;

template<typename T>
constexpr T swap_bytes_integral(T value) noexcept;

void swap_bytes_inplace(void* data, size_t size);

// 字节序修正
void fix_endianness(Value& value, bool reverse_endianness) noexcept;

// 网络字节序
template<SwappableIntegral T>
constexpr T host_to_network(T value) noexcept;

template<SwappableIntegral T>
constexpr T network_to_host(T value) noexcept;

// 小端序转换
template<SwappableIntegral T>
constexpr T host_to_little_endian(T value) noexcept;

template<SwappableIntegral T>
constexpr T little_endian_to_host(T value) noexcept;
```

#### 概念

```cpp
template<typename T>
concept SwappableIntegral = std::integral<T> && 
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
```

---

## process_checker

### 枚举: `ProcessState`

```cpp
enum class ProcessState { RUNNING, ERROR, DEAD, ZOMBIE };
```

### 类: `ProcessChecker`

#### ProcessChecker 静态方法

```cpp
static ProcessState check_process(pid_t pid);
static bool is_process_dead(pid_t pid);
```

---

## sets

### 结构体: `Set`

```cpp
struct Set {
    std::vector<size_t> buf;
    
    size_t size() const;
    void clear();
    static int cmp(const size_t& i1, const size_t& i2);
};
```

#### Set 函数

```cpp
bool parse_uintset(std::string_view lptr, Set& set, size_t maxSZ);
```

---

## show_message

### 枚举: `MessageType`

```cpp
enum class MessageType : uint8_t {
    INFO,    // 信息消息
    WARN,    // 警告消息
    ERROR,   // 错误消息
    DEBUG,   // 调试消息
    USER     // 用户消息
};
```

### 结构体: `MessageContext`

```cpp
struct MessageContext {
    bool debugMode = false;    // 调试模式
    bool backendMode = false;  // 后端模式
};
```

### 类: `MessagePrinter`

#### 构造函数

```cpp
MessagePrinter(MessageContext ctx = {});
```

#### 方法

```cpp
template<typename... Args>
void print(MessageType type, std::string_view fmt, Args&&... args) const;

template<typename... Args>
void info(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void warn(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void error(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void debug(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void user(std::string_view fmt, Args&&... args) const;
```

---

## targetmem

### 结构体: `OldValueAndMatchInfo`

```cpp
struct OldValueAndMatchInfo {
    uint8_t old_value;      // 原始字节值
    MatchFlags match_info;  // 匹配类型和状态标志
};
```

### 类: `MatchesAndOldValuesSwath`

#### 成员变量

```cpp
void* firstByteInChild = nullptr;                    // 起始地址
std::vector<OldValueAndMatchInfo> data;              // 匹配数据
```

#### MatchesAndOldValuesSwath 方法

```cpp
void addElement(void* addr, uint8_t byte, MatchFlags matchFlags);
std::string toPrintableString(size_t idx, size_t len) const;
std::string toByteArrayText(size_t idx, size_t len) const;
```

### 类: `MatchesAndOldValuesArray`

#### MatchesAndOldValuesArray 成员变量

```cpp
std::vector<MatchesAndOldValuesSwath> swaths;        // 内存区域集合
```

#### MatchesAndOldValuesArray 方法

```cpp
std::optional<size_t> findSwathIndex(void* addr) const;
std::optional<size_t> findElementIndex(void* addr) const;
const OldValueAndMatchInfo* getElement(void* addr) const;
OldValueAndMatchInfo* getElement(void* addr);
void clear();
size_t size() const;
```

---

## value

### 枚举: `MatchFlags`

```cpp
enum class [[gnu::packed]] MatchFlags : uint16_t {
    EMPTY = 0,

    // 基本数值类型
    U8B = 1 << 0,   // 无符号 8 位
    S8B = 1 << 1,   // 有符号 8 位
    U16B = 1 << 2,  // 无符号 16 位
    S16B = 1 << 3,  // 有符号 16 位
    U32B = 1 << 4,  // 无符号 32 位
    S32B = 1 << 5,  // 有符号 32 位
    U64B = 1 << 6,  // 无符号 64 位
    S64B = 1 << 7,  // 有符号 64 位

    // 浮点类型
    F32B = 1 << 8,  // 32 位浮点
    F64B = 1 << 9,  // 64 位浮点

    // 复合类型
    I8B = U8B | S8B,      // 任意 8 位整数
    I16B = U16B | S16B,   // 任意 16 位整数
    I32B = U32B | S32B,   // 任意 32 位整数
    I64B = U64B | S64B,   // 任意 64 位整数

    INTEGER = I8B | I16B | I32B | I64B,  // 所有整数类型
    FLOAT = F32B | F64B,                  // 所有浮点类型
    ALL = INTEGER | FLOAT,                // 所有支持的类型

    // 基于字节的分组
    B8 = I8B,           // 8 位块
    B16 = I16B,         // 16 位块
    B32 = I32B | F32B,  // 32 位块
    B64 = I64B | F64B,  // 64 位块

    MAX = 0xffffU  // 最大标志值
};
```

### 结构体: `Value`

#### Value 成员变量

```cpp
std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
             uint64_t, float, double, std::array<uint8_t, sizeof(int64_t)>,
             std::array<char, sizeof(int64_t)>> value;

MatchFlags flags = MatchFlags::EMPTY;
```

#### Value 静态方法

```cpp
constexpr static void zero(Value& val);
```

---

## 详细函数说明

### endianness 模块

#### `is_big_endian()`

检测系统是否为大端序。

**返回值:** `true` 如果系统为大端序，否则 `false`

#### `is_little_endian()`

检测系统是否为小端序。

**返回值:** `true` 如果系统为小端序，否则 `false`

#### `swap_bytes(T value)`

交换指定类型值的字节序。

**模板参数:** `T` - 整数类型 (uint8_t, uint16_t, uint32_t, uint64_t)

**参数:** `value` - 要交换字节序的值

**返回值:** 交换字节序后的值

#### `swap_bytes_inplace(void* data, size_t size)`

原地交换内存中数据的字节序。

**参数:**

- `data` - 指向数据的指针
- `size` - 数据大小（字节）

#### `fix_endianness(Value& value, bool reverse_endianness)`

修正 Value 对象的字节序。

**参数:**

- `value` - 要修正的 Value 对象
- `reverse_endianness` - 是否反转字节序

### process_checker 模块

#### `ProcessChecker::check_process(pid_t pid)`

检查指定进程的状态。

**参数:** `pid` - 进程 ID

**返回值:** `ProcessState` 枚举值表示进程状态

#### `ProcessChecker::is_process_dead(pid_t pid)`

检查进程是否已死亡。

**参数:** `pid` - 进程 ID

**返回值:** `true` 如果进程已死亡或不存在，否则 `false`

### sets 模块

#### `Set::size()`

获取集合中的元素数量。

**返回值:** 元素数量

#### `Set::clear()`

清空集合中的所有元素。

#### `Set::cmp(const size_t& i1, const size_t& i2)`

比较两个 size_t 值。

**参数:**

- `i1` - 第一个值
- `i2` - 第二个值

**返回值:** 比较结果（-1, 0, 1）

#### `parse_uintset(std::string_view lptr, Set& set, size_t maxSZ)`

解析无符号整数集合表达式。

**参数:**

- `lptr` - 要解析的字符串
- `set` - 用于存储结果的 Set 对象
- `maxSZ` - 最大允许值

**返回值:** `true` 如果解析成功，否则 `false`

### show_message 模块

#### `MessagePrinter::print(MessageType type, std::string_view fmt, Args&&... args)`

打印格式化的消息。

**参数:**

- `type` - 消息类型
- `fmt` - 格式字符串
- `args` - 格式化参数

#### `MessagePrinter::info/warn/error/debug/user(fmt, args...)`

便捷方法，用于打印特定类型的消息。

### targetmem 模块

#### `MatchesAndOldValuesSwath::addElement(void* addr, uint8_t byte, MatchFlags matchFlags)`

向 swath 添加新的内存匹配。

**参数:**

- `addr` - 内存地址
- `byte` - 字节值
- `matchFlags` - 匹配标志

#### `MatchesAndOldValuesSwath::toPrintableString(size_t idx, size_t len)`

将内存字节转换为可打印字符串。

**参数:**

- `idx` - 起始索引
- `len` - 长度

**返回值:** 可打印字符串

#### `MatchesAndOldValuesArray::findSwathIndex(void* addr)`

查找包含指定地址的 swath 索引。

**参数:** `addr` - 内存地址

**返回值:** 包含该地址的 swath 索引，如果未找到则返回 std::nullopt

#### `MatchesAndOldValuesArray::getElement(void* addr)`

获取指定地址的元素数据。

**参数:** `addr` - 内存地址

**返回值:** 指向 OldValueAndMatchInfo 的指针，如果未找到则返回 nullptr

### value 模块

#### `Value::zero(Value& val)`

将 Value 对象清零。

**参数:** `val` - 要清零的 Value 对象

---

## API 使用示例

### 基本用法

```cpp
import endianness;
import process_checker;
import sets;
import show_message;
import targetmem;
import value;

// 检查字节序
if (endianness::is_little_endian()) {
    std::cout << "系统为小端序" << std::endl;
}

// 检查进程状态
pid_t pid = 1234;
ProcessState state = ProcessChecker::check_process(pid);
if (state == ProcessState::RUNNING) {
    std::cout << "进程正在运行" << std::endl;
}

// 解析集合
Set mySet;
parse_uintset("1,2,3,4,5", mySet, 100);

// 创建消息打印机
MessageContext ctx;
ctx.debugMode = true;
MessagePrinter printer(ctx);
printer.info("开始处理");

// 创建内存匹配数组
MatchesAndOldValuesArray matches;

// 创建值对象
Value searchValue;
searchValue.value = static_cast<int32_t>(42);
searchValue.flags = MatchFlags::S32B;
```

### 高级用法

```cpp
// 字节序转换
uint32_t value = 0x12345678;
uint32_t swapped = endianness::swap_bytes(value);

// 进程监控循环
while (true) {
    if (ProcessChecker::is_process_dead(targetPid)) {
        std::cout << "目标进程已终止" << std::endl;
        break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 复杂集合解析
Set complexSet;
parse_uintset("1,5,10..15,0x20", complexSet, 1000);

// 格式化消息
printer.info("找到 {} 个匹配项", matchCount);
printer.warn("内存使用率: {:.2f}%", memoryUsage);
printer.error("在地址 0x{:08x} 处读取失败", address);

// 内存匹配操作
void* targetAddr = (void*)0x1000;
auto swathIndex = matches.findSwathIndex(targetAddr);
if (swathIndex) {
    const auto* element = matches.getElement(targetAddr);
    if (element) {
        std::cout << "旧值: 0x" << std::hex << (int)element->old_value << std::endl;
    }
}
```
