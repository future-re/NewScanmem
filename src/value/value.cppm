module;
#include <cstdint>
#include <cstring>
#include <vector>

export module value;

export import value.flags;

// Value: 底层字节容器，只负责存储原始字节数据
// 不包含任何标量类型解析逻辑，那些由 value.scalar 模块处理
export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    Value() = default;

    // 从原始数据构造
    explicit Value(const std::uint8_t* data, std::size_t len)
        : bytes(data, data + len) {}

    explicit Value(std::vector<std::uint8_t> data) : bytes(std::move(data)) {}

    // 设置字节内容
    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }

    void setBytes(std::vector<std::uint8_t>&& data) { bytes = std::move(data); }

    // 获取字节视图
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto data() noexcept -> std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return bytes.empty();
    }

    // 清空
    void clear() {
        bytes.clear();
        flags = MatchFlags::EMPTY;
    }
};
