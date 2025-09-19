module;

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// export module targetmem;

// // import value.value;
// import show_message;

// // Interface: exported types and functions
// export {
//     struct RangeMark {
//         // 以 swath.data 为基准的起始索引与长度
//         size_t startIndex;
//         size_t length;
//         MatchFlags flags;
//     };

//     struct OldValueAndMatchInfo {
//         uint8_t oldValue;
//         MatchFlags matchInfo;
//     };

//     class MatchesAndOldValuesSwath {
//        public:
//         void* firstByteInChild = nullptr;
//         std::vector<OldValueAndMatchInfo> data;
//         // 记录按区间的标记（辅助结构，便于 UI 或后处理）
//         std::vector<RangeMark> rangeMarks;

//         MatchesAndOldValuesSwath() = default;

//         /*
//          * addElement(addr, byte, matchFlags)
//          *
//          * 向 swath 追加一个字节（OldValueAndMatchInfo）。
//          * - addr: 该字节在目标进程中的地址（字节地址）。
//          * - byte: 读取到的字节值。
//          * - matchFlags: 该字节的匹配状态（MatchFlags）。
//          *
//          * 语义:
//          *  - 如果 data 为空，则将 firstByteInChild 设为 addr，之后按顺序
//          * push_back。
//          *  - 依赖调用者按地址顺序添加字节以保持 firstByteInChild 与 data
//          * 的对应关系。
//          */
//         void addElement(void* addr, uint8_t byte, MatchFlags matchFlags) {
//             if (data.empty()) {
//                 firstByteInChild = addr;
//             }
//             data.push_back({byte, matchFlags});
//         }

//         // 以索引区间 [startIndex, startIndex+length) 方式批量标记
//         void markRangeByIndex(size_t startIndex, size_t length,
//                               MatchFlags flags) {
//             if (length == 0 || startIndex >= data.size()) {
//                 return;
//             }
//             size_t endIndex = std::min(startIndex + length, data.size());
//             for (size_t i = startIndex; i < endIndex; ++i) {
//                 data[i].matchInfo = data[i].matchInfo | flags;
//             }
//             rangeMarks.push_back({startIndex, endIndex - startIndex, flags});
//         }

//         // 以目标地址区间 [addr, addr+length) 方式批量标记
//         void markRangeByAddress(void* addr, size_t length, MatchFlags flags)
//         {
//             if ((firstByteInChild == nullptr) || length == 0) {
//                 return;
//             }
//             const auto* base = static_cast<const char*>(firstByteInChild);
//             const auto* curr = static_cast<const char*>(addr);
//             if (curr < base) {
//                 return;
//             }
//             auto offset = static_cast<size_t>(curr - base);
//             markRangeByIndex(offset, length, flags);
//         }

//         /*
//          * toPrintableString(idx, len)
//          *
//          * 将从 idx 起最多 len
//          * 个字节转换为用于显示的可打印字符序列（不可打印字符用 '.' 代替）。
//          * 返回一个 std::string，方便在 UI/log 中显示区域内的文本表示。
//          */
//         [[nodiscard]] auto toPrintableString(size_t idx, size_t len) const
//             -> std::string {
//             std::ostringstream oss;
//             size_t count = std::min(len, data.size() - idx);
//             for (size_t i = 0; i < count; ++i) {
//                 auto byteVal = data[idx + i].oldValue;
//                 oss << ((std::isprint(byteVal) != 0)
//                             ? static_cast<char>(byteVal)
//                             : '.');
//             }
//             return oss.str();
//         }

//         /*
//          * toByteArrayText(idx, len)
//          *
//          * 将从 idx 起最多 len
//          * 个字节转换为十六进制字符串（以空格分隔），便于调试查看原始字节。
//          */
//         [[nodiscard]] auto toByteArrayText(size_t idx, size_t len) const
//             -> std::string {
//             std::ostringstream oss;
//             size_t count = std::min(len, data.size() - idx);
//             for (size_t i = 0; i < count; ++i) {
//                 oss << std::hex << static_cast<int>(data[idx + i].oldValue);
//                 if (i + 1 < count) {
//                     oss << " ";
//                 }
//             }
//             return oss.str();
//         }
//     };

//     /*
//      * MatchesAndOldValuesArray
//      *
//      * 管理多个 swath（连续字节片段）的集合，用于保存目标进程中多个区域的
//      * 历史字节值与匹配信息。
//      *
//      * 字段:
//      *  - maxNeededBytes:
//      为了预分配或限制使用的最大字节数（语义由调用方定义）。
//      *  - swaths: 各个 MatchesAndOldValuesSwath 实例的容器。
//      */
//     class MatchesAndOldValuesArray {
//        public:
//         size_t maxNeededBytes;
//         std::vector<MatchesAndOldValuesSwath> swaths;

//         MatchesAndOldValuesArray(size_t maxBytes) : maxNeededBytes(maxBytes)
//         {}

//         /*
//          * addSwath(swath)
//          *
//          * 将一个完整的 swath 追加到数组末尾。
//          * 语义: 直接拷贝 swath（调用方可继续使用原 swath）；保持 swath
//          * 顺序即地址顺序（若 caller 保证）。
//          */
//         void addSwath(const MatchesAndOldValuesSwath& swath) {
//             swaths.push_back(swath);
//         }

//         /*
//          * nthMatch(n)
//          *
//          * 返回第 n 个被标记为匹配（matchInfo != MatchFlags::EMPTY）的位置。
//          *
//          * 返回值:
//          *  - 成功: std::pair<swath_ptr, index>，其中 swath_ptr
//          指向包含该匹配的
//          *    MatchesAndOldValuesSwath，index 是 swath.data
//          中匹配元素的下标。
//          *  - 失败: std::nullopt（当 n 超过匹配总数时）。
//          */
//         auto nthMatch(size_t n)
//             -> std::optional<std::pair<MatchesAndOldValuesSwath*, size_t>> {
//             size_t count = 0;
//             for (auto& swath : swaths) {
//                 for (size_t i = 0; i < swath.data.size(); ++i) {
//                     if (swath.data[i].matchInfo != MatchFlags::EMPTY) {
//                         if (count == n) {
//                             return std::make_pair(&swath, i);
//                         }
//                         ++count;
//                     }
//                 }
//             }
//             return std::nullopt;
//         }

//         /*
//          * 删除给定地址区间 [start, end)
//          * 内的所有已记录字节（包括匹配和非匹配）。
//          *
//          * 参数:
//          *  - start, end: 要删除的地址区间（半开区间，包含 start 不包含
//          end）。
//          *  - numMatches: 输出参数，删除过程中遇到的匹配（matchInfo !=
//          * EMPTY）个数。
//          *
//          * 逻辑:
//          *  - 遍历每个 swath 的 data（按字节存储的 OldValueAndMatchInfo
//          向量）。
//          *  - 对每个元素通过索引计算其在目标进程内的实际地址：
//          *      addr = firstByteInChild + (元素在 data 中的偏移)
//          *    这里使用 &info - swath.data.data()
//          * 得到元素索引（size_t），然后加到 firstByteInChild。
//          *  - 如果 addr 落在 [start, end)
//          * 内，则将该元素视为要删除；同时如果它是一个匹配项则增加
//          numMatches。
//          *  - 使用 std::ranges::remove_if
//          * 进行“移除-擦除”操作以高效删除满足条件的元素。
//          */
//         void deleteInAddressRange(void* start, void* end,
//                                   unsigned long& numMatches) {
//             numMatches = 0;
//             for (auto& swath : swaths) {
//                 auto iter = std::ranges::remove_if(
//                     swath.data, [&](const OldValueAndMatchInfo& info) {
//                         void* addr =
//                             static_cast<char*>(swath.firstByteInChild) +
//                             (&info - swath.data.data());
//                         if (addr >= start && addr < end) {
//                             if (info.matchInfo != MatchFlags::EMPTY) {
//                                 ++numMatches;
//                             }
//                             return true;
//                         }
//                         return false;
//                     });
//                 swath.data.erase(iter.begin(), swath.data.end());

//                 // 同步清理 rangeMarks 中完全落在删除区间内的标记（保守做法）
//                 auto riter = std::ranges::remove_if(
//                     swath.rangeMarks, [&](const RangeMark& mark) {
//                         void* rstart =
//                             static_cast<char*>(swath.firstByteInChild) +
//                             mark.startIndex;
//                         void* rend = static_cast<char*>(rstart) +
//                         mark.length; return (rstart >= start && rend <= end);
//                     });
//                 swath.rangeMarks.erase(riter.begin(),
//                 swath.rangeMarks.end());
//             }
//         }
//     };
// }
