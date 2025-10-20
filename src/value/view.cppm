module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <type_traits>

export module value.view;

import utils.endianness;
import value.buffer;

// 字节序标记（目标端序）
export enum class Endian { HOST, LITTLE, BIG };

// 非拥有型视图：对已有字节的零拷贝观察（以 buffer+offset 表示）
export struct MemView {
    std::shared_ptr<const ByteBuffer> buf;
    size_t off{};
    size_t len{};

    MemView() = default;

    explicit MemView(size_t n) : buf(nullptr), len(n) {}

    MemView(std::shared_ptr<const ByteBuffer> buffer, size_t offset, size_t n)
        : buf(std::move(buffer)), off(offset), len(n) {}

    // Factory to construct a MemView from a shared_ptr to ByteBuffer
    static auto fromBuffer(std::shared_ptr<const ByteBuffer> buffer,
                           size_t offset, size_t n) noexcept -> MemView {
        return {std::move(buffer), offset, n};
    }

    [[nodiscard]] auto size() const noexcept -> size_t { return len; }

    [[nodiscard]] auto data() const noexcept -> const uint8_t* {
        return buf ? buf->ptr() + off : nullptr;
    }

    // 提取标量值；当 eType 指示源数据为 BIG/LITTLE 时，转换为 host
    template <typename T>
    [[nodiscard]] auto tryGet(Endian eType = Endian::HOST) const noexcept
        -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        const uint8_t* point = data();
        if (point == nullptr || len < sizeof(T)) {
            return std::nullopt;
        }
        T out;
        std::memcpy(&out, point, sizeof(T));
        if (eType == Endian::BIG) {
            out = endianness::networkToHost(out);
        } else if (eType == Endian::LITTLE) {
            out = endianness::littleEndianToHost(out);
        }
        return out;
    }

    // 安全子视图
    [[nodiscard]] auto subview(size_t off, size_t len) const noexcept
        -> std::optional<MemView> {
        if (off + len > this->len) {
            return std::nullopt;
        }
        return MemView{buf, this->off + off, len};
    }

    // 按偏移读取
    template <typename T>
    [[nodiscard]] auto tryGetAt(size_t off,
                                Endian eType = Endian::HOST) const noexcept
        -> std::optional<T> {
        if (off + sizeof(T) > len) {
            return std::nullopt;
        }
        MemView memView{buf, this->off + off, sizeof(T)};
        return memView.tryGet<T>(eType);
    }
};
