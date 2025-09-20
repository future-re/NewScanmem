module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>

export module value.view;

import endianness;

// 字节序标记（目标端序）
export enum class Endian { HOST, LITTLE, BIG };

// 非拥有型视图：对已有字节的零拷贝观察（指针+长度）
export struct MemView {
    const uint8_t* ptr{};
    size_t len{};

    MemView() = default;
    explicit MemView(const uint8_t* point, size_t n) : ptr(point), len(n) {}

    [[nodiscard]] auto size() const noexcept -> size_t { return len; }
    [[nodiscard]] auto data() const noexcept -> const uint8_t* { return ptr; }

    // 提取标量值；当 eType 指示源数据为 BIG/LITTLE 时，转换为 host
    template <typename T>
    [[nodiscard]] auto tryGet(Endian eType = Endian::HOST) const noexcept
        -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        if (ptr == nullptr || len < sizeof(T)) {
            return std::nullopt;
        }

        T out;
        std::memcpy(&out, ptr, sizeof(T));
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
        return MemView{ptr + off, len};
    }

    // 不安全切片（越界返回空视图）
    [[nodiscard]] auto slice(size_t off, size_t len) const noexcept -> MemView {
        if (off + len > this->len) {
            return MemView{};
        }
        return MemView{ptr + off, len};
    }

    // 按偏移读取
    template <typename T>
    [[nodiscard]] auto tryGetAt(size_t off,
                                Endian eType = Endian::HOST) const noexcept
        -> std::optional<T> {
        if (off + sizeof(T) > len) {
            return std::nullopt;
        }
        MemView memView{ptr + off, sizeof(T)};
        return memView.tryGet<T>(eType);
    }
};
