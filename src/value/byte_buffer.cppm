
module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>
export module value.byte_buffer;

import endianness;

// 字节排序顺序， 主机字节序， 小端字节序， 大端字节序
export enum class Endian { HOST, LITTLE, BIG };

export using ByteSpan = std::span<const uint8_t>;

// 底层容器
export struct ByteBuffer {
    std::vector<uint8_t> data;

    ByteBuffer() = default;
    explicit ByteBuffer(size_t n) : data(n) {}
    ByteBuffer(std::span<const uint8_t> sVal)
        : data(sVal.begin(), sVal.end()) {}

    // 添加数据到缓冲区末尾
    void append(std::span<const uint8_t> sVal) {
        data.insert(data.end(), sVal.begin(), sVal.end());
    }

    // 预留空间
    void reserve(size_t n) { data.reserve(n); }

    // 清空缓冲区
    void clear() noexcept { data.clear(); }

    // 获取缓冲区大小
    [[nodiscard]] auto size() const noexcept -> size_t { return data.size(); }

    // 获取底层数据指针
    auto ptr() noexcept -> uint8_t* { return data.data(); }
    [[nodiscard]] auto ptr() const noexcept -> const uint8_t* {
        return data.data();
    }

    // 获取只读视图
    [[nodiscard]] auto view() const noexcept -> ByteSpan {
        return ByteSpan{data.data(), data.size()};
    }
};

// 非拥有型视图：负责对已有数据的轻量级操作
export struct MemView {
    ByteSpan buf;

    MemView() = default;
    explicit MemView(ByteSpan buf) : buf(buf) {}

    // 获取视图大小
    [[nodiscard]] auto size() const noexcept -> size_t { return buf.size(); }

    // 获取底层数据指针
    [[nodiscard]] auto data() const noexcept -> const uint8_t* {
        return buf.data();
    }

    // 从视图中提取标量值，并根据指定字节序进行转换
    template <typename T>
    [[nodiscard]] auto tryGet(Endian eType = Endian::HOST) const noexcept
        -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        if (buf.size() < sizeof(T)) {
            return std::nullopt;
        }

        T out;
        std::memcpy(&out, buf.data(), sizeof(T));

        // 根据指定的字节序进行转换
        if (eType == Endian::BIG) {
            out = endianness::hostToNetwork(out);
        } else if (eType == Endian::LITTLE) {
            out = endianness::hostToLittleEndian(out);
        }

        return out;
    }

    // 获取原始字节片段
    [[nodiscard]] auto slice(size_t off, size_t len) const noexcept
        -> ByteSpan {
        if (off + len > buf.size()) {
            return {};
        }
        return ByteSpan{buf.data() + off, len};
    }
};