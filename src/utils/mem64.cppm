module;

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

export module utils.mem64;

import value; // MemView, Endian helpers

// Mem64: 统一的“当前内存字节”容器
// - 用作扫描时的内存视图载体（来自目标进程读取的字节）
// - 也可承载用户输入转成的原始字节（便于字节级匹配）
// - 可与 value 模块的视图/标量工具协同工作
export struct Mem64 {
   private:
    std::shared_ptr<ByteBuffer> m_buf;

   public:
    Mem64() : m_buf(std::make_shared<ByteBuffer>()) {}
    explicit Mem64(std::size_t n) : m_buf(std::make_shared<ByteBuffer>(n)) {}
    Mem64(const std::uint8_t* point, std::size_t n)
        : m_buf(std::make_shared<ByteBuffer>(point, n)) {}
    explicit Mem64(std::span<const std::uint8_t> span)
        : m_buf(std::make_shared<ByteBuffer>()) {
        setBytes(span);
    }
    explicit Mem64(const std::string& stringInput)
        : m_buf(std::make_shared<ByteBuffer>()) {
        setString(stringInput);
    }

    // 基本信息/视图
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return m_buf->size();
    }
    [[nodiscard]] auto empty() const noexcept -> bool { return size() == 0; }
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return m_buf->ptr();
    }
    [[nodiscard]] auto data() noexcept -> std::uint8_t* { return m_buf->ptr(); }

    // 零拷贝视图（value.view::MemView）
    [[nodiscard]] auto view() const noexcept -> MemView {
        auto cptr = std::const_pointer_cast<const ByteBuffer>(m_buf);
        return MemView::fromBuffer(cptr, 0, m_buf->size());
    }

    // 标准只读 span 视图
    [[nodiscard]] auto bytes() const noexcept -> std::span<const std::uint8_t> {
        return {data(), size()};
    }

    // 赋值/写入接口
    void clear() { m_buf->clear(); }
    void reserve(std::size_t n) { m_buf->reserve(n); }

    void setBytes(const std::uint8_t* point, std::size_t n) {
        m_buf->clear();
        m_buf->append(point, n);
    }
    void setBytes(std::span<const std::uint8_t> span) {
        setBytes(span.data(), span.size());
    }
    void setBytes(const std::vector<std::uint8_t>& val) {
        setBytes(val.data(), val.size());
    }
    void setString(const std::string& stringInput) {
        const auto* point =
            std::bit_cast<const std::uint8_t*>(stringInput.data());
        setBytes(point, stringInput.size());
    }

    template <typename T>
    void setScalar(const T& val) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "setScalar requires trivially copyable T");
        m_buf->clear();
        (void)m_buf->appendValue<T>(val);
    }

    // 读取接口：尝试从起始位置解释为标量
    template <typename T>
    [[nodiscard]] auto tryGet(Endian endian = Endian::HOST) const noexcept
        -> std::optional<T> {
        return view().tryGet<T>(endian);
    }

    // 读取接口：不带 optional 的版本（用于旧实现迁移），失败时抛出
    template <typename T>
    [[nodiscard]] auto get(Endian endian = Endian::HOST) const -> T {
        auto opt = tryGet<T>(endian);
        if (!opt) {
            throw std::runtime_error("Mem64::get<T>() insufficient bytes");
        }
        return *opt;
    }
};
