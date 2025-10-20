module;

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module value;

export import value.buffer;
export import value.view;
export import value.cursor;
export import value.scalar;
export import value.flags;

import value.buffer;
import value.view;

// 最小可用的 Value / UserValue 定义，满足 scan.* 与 helpers 依赖
export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::shared_ptr<ByteBuffer> buf;

    Value() : buf(std::make_shared<ByteBuffer>()) {}

    // 只读视图
    [[nodiscard]] auto view() const noexcept -> MemView {
        auto cptr = std::const_pointer_cast<const ByteBuffer>(buf);
        return MemView::fromBuffer(cptr, 0, buf->size());
    }

    // 设置字节内容（便于测试或集成其它模块）
    void setBytes(const std::uint8_t* data, std::size_t len) {
        buf->clear();
        buf->append(data, len);
    }
};

export struct UserValue {
    // 数值（低/高，用于 range）
    int8_t  s8 = 0,  s8h = 0;
    uint8_t u8 = 0,  u8h = 0;
    int16_t s16 = 0, s16h = 0;
    uint16_t u16 = 0, u16h = 0;
    int32_t s32 = 0, s32h = 0;
    uint32_t u32 = 0, u32h = 0;
    int64_t s64 = 0, s64h = 0;
    uint64_t u64 = 0, u64h = 0;
    float f32 = 0.0F, f32h = 0.0F;
    double f64 = 0.0, f64h = 0.0;

    // 字节数组/字符串与掩码
    std::optional<std::vector<std::uint8_t>> bytearrayValue;
    std::optional<std::vector<std::uint8_t>> byteMask;  // 0xFF=fixed, 0x00=wildcard
    std::string stringValue;

    // 有效标志（指示上面哪个类型/宽度有效）
    MatchFlags flags = MatchFlags::EMPTY;
};
