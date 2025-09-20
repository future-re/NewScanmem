module;

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

export module value.buffer;

// 单纯的字节容器：负责拥有与管理底层存储
export struct ByteBuffer {
    std::vector<uint8_t> data;
    bool frozen = false;

    ByteBuffer() = default;
    explicit ByteBuffer(size_t n) : data(n) {}
    ByteBuffer(const uint8_t* point, size_t n) : data(point, point + n) {}

    void append(const uint8_t* point, size_t n) {
        if (frozen) {
            throw std::logic_error("ByteBuffer is frozen");
        }
        data.insert(data.end(), point, point + n);
    }

    template <typename T>
    auto appendValue(const T& val) -> size_t {
        static_assert(std::is_trivially_copyable_v<T>,
                      "appendValue requires trivially copyable T");
        if (frozen) {
            throw std::logic_error("ByteBuffer is frozen");
        }
        auto bytes = std::bit_cast<std::array<uint8_t, sizeof(T)>>(val);
        const size_t START = data.size();
        data.insert(data.end(), bytes.begin(), bytes.end());
        return START;
    }

    void reserve(size_t n) {
        if (frozen) {
            throw std::logic_error("ByteBuffer is frozen");
        }
        data.reserve(n);
    }
    void clear() {
        if (frozen) {
            throw std::logic_error("ByteBuffer is frozen");
        }
        data.clear();
    }

    [[nodiscard]] auto size() const noexcept -> size_t { return data.size(); }
    auto ptr() noexcept -> uint8_t* { return data.data(); }
    [[nodiscard]] auto ptr() const noexcept -> const uint8_t* {
        return data.data();
    }

    void freeze() noexcept { frozen = true; }
    [[nodiscard]] auto isFrozen() const noexcept -> bool { return frozen; }
};
