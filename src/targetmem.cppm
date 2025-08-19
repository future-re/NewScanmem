module;

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

export module targetmem;

import value;
import show_message;

using std::size_t;

// 单个匹配信息（POD）
export struct OldValueAndMatchInfo {
    uint8_t old_value{};
    MatchFlags match_info{};
};

// 一段连续内存的匹配信息（swath），C++ 内部表示，自动管理内存
export class MatchesAndOldValuesSwath {
   public:
    MatchesAndOldValuesSwath() = default;
    MatchesAndOldValuesSwath(void* ptr, size_t n)
        : firstByteInChild_{ptr}, data_{n} {}

    [[nodiscard]] size_t numberOfBytes() const noexcept { return data_.size(); }

    OldValueAndMatchInfo& operator[](size_t idx) noexcept { return data_[idx]; }
    const OldValueAndMatchInfo& operator[](size_t idx) const noexcept {
        return data_[idx];
    }

    [[nodiscard]] void* firstByteInChild() const noexcept {
        return firstByteInChild_;
    }
    void setFirstByteInChild(void* ptr) noexcept { firstByteInChild_ = ptr; }

    // 迭代器访问（[[nodiscard]] 提示不要忽略返回值）
    [[nodiscard]] auto begin() noexcept { return data_.begin(); }
    [[nodiscard]] auto end() noexcept { return data_.end(); }
    [[nodiscard]] auto begin() const noexcept { return data_.begin(); }
    [[nodiscard]] auto end() const noexcept { return data_.end(); }

    // 高效添加
    void emplace_back(uint8_t oldValue, MatchFlags flags) {
        data_.emplace_back(
            OldValueAndMatchInfo{.old_value = oldValue, .match_info = flags});
    }

   private:
    void* firstByteInChild_ = nullptr;
    std::vector<OldValueAndMatchInfo> data_;
};

// 主数组，包含多个 swath（仅 C++ 表示）
export class MatchesAndOldValuesArray {
   public:
    MatchesAndOldValuesArray() = default;

    [[nodiscard]] size_t swathCount() const noexcept { return swaths_.size(); }
    [[nodiscard]] size_t maxNeededBytes() const noexcept {
        return maxNeededBytes_;
    }
    void setMaxNeededBytes(size_t n) noexcept { maxNeededBytes_ = n; }

    [[nodiscard]] auto begin() noexcept { return swaths_.begin(); }
    [[nodiscard]] auto end() noexcept { return swaths_.end(); }
    [[nodiscard]] auto begin() const noexcept { return swaths_.begin(); }
    [[nodiscard]] auto end() const noexcept { return swaths_.end(); }

    void pushBack(std::shared_ptr<MatchesAndOldValuesSwath> swath) {
        swaths_.push_back(std::move(swath));
    }

    [[nodiscard]] std::shared_ptr<MatchesAndOldValuesSwath>& operator[](
        size_t idx) noexcept {
        return swaths_[idx];
    }
    [[nodiscard]] const std::shared_ptr<MatchesAndOldValuesSwath>& operator[](
        size_t idx) const noexcept {
        return swaths_[idx];
    }

   private:
    std::vector<std::shared_ptr<MatchesAndOldValuesSwath>> swaths_;
    size_t maxNeededBytes_ = 0;
};

// 匹配位置
export struct MatchLocation {
    std::shared_ptr<MatchesAndOldValuesSwath> swath;
    size_t index = 0;
};

// 工具函数 —— 标注 noexcept 和 [[nodiscard]]
export [[nodiscard]] inline size_t indexOfLastElement(
    const MatchesAndOldValuesSwath& swath) noexcept {
    return (swath.numberOfBytes() != 0U) ? (swath.numberOfBytes() - 1U) : 0U;
}

export [[nodiscard]] inline void* remoteAddressOfNthElement(
    const MatchesAndOldValuesSwath& swath, size_t n) noexcept {
    return static_cast<std::byte*>(swath.firstByteInChild()) + n;
}

export [[nodiscard]] inline void* remoteAddressOfLastElement(
    const MatchesAndOldValuesSwath& swath) noexcept {
    return remoteAddressOfNthElement(swath, indexOfLastElement(swath));
}

export [[nodiscard]] inline OldValueAndMatchInfo* localAddressBeyondNthElement(
    MatchesAndOldValuesSwath& swath, size_t n) noexcept {
    return (n + 1U < swath.numberOfBytes()) ? &swath[n + 1U] : nullptr;
}

export [[nodiscard]] inline OldValueAndMatchInfo* localAddressBeyondLastElement(
    MatchesAndOldValuesSwath& swath) noexcept {
    return localAddressBeyondNthElement(swath, indexOfLastElement(swath));
}

// 添加元素（在 C++ 表示下更简单、安全）
export inline void addElement(
    MatchesAndOldValuesArray& /*array*/,
    const std::shared_ptr<MatchesAndOldValuesSwath>& swath, void* remoteAddress,
    uint8_t newByte, MatchFlags newFlags) noexcept {
    if (!swath) {
        return;
    }
    swath->setFirstByteInChild(remoteAddress);
    swath->emplace_back(newByte, newFlags);
}

// 取值相关：将 swath 的字节转成 Value（保留原有逻辑）
export [[nodiscard]] inline Value dataToValAux(
    const MatchesAndOldValuesSwath& swath, size_t index,
    size_t swathLength) noexcept {
    Value val{};
    size_t maxBytes = (swathLength > index) ? (swathLength - index) : 0U;
    using UT = std::underlying_type_t<MatchFlags>;
    UT flags_ut = static_cast<UT>(0xffffU);

    maxBytes = std::min<size_t>(maxBytes, 8U);
    if (maxBytes < 8U) {
        flags_ut &= ~static_cast<UT>(MatchFlags::B64);
    }
    if (maxBytes < 4U) {
        flags_ut &= ~static_cast<UT>(MatchFlags::B32);
    }
    if (maxBytes < 2U) {
        flags_ut &= ~static_cast<UT>(MatchFlags::B16);
    }

    if (maxBytes < 1U) {
        val.flags = MatchFlags::Empty;
    } else {
        val.flags = static_cast<MatchFlags>(flags_ut);
    }

    for (size_t i = 0U; i < maxBytes; ++i) {
        val.bytes[i] = swath[index + i].old_value;
    }

    val.flags = static_cast<MatchFlags>(
        static_cast<UT>(val.flags) & static_cast<UT>(swath[index].match_info));
    return val;
}

export [[nodiscard]] inline Value dataToVal(
    const MatchesAndOldValuesSwath& swath, size_t index) noexcept {
    return dataToValAux(swath, index, swath.numberOfBytes());
}