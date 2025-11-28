module;

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

export module utils.mem64;

export struct Mem64 {
   private:
    std::vector<std::uint8_t> m_buf;

   public:
    Mem64() = default;
    explicit Mem64(std::size_t n) : m_buf(n) {}
    Mem64(const std::uint8_t* point, std::size_t n) : m_buf(point, point + n) {}
    explicit Mem64(std::span<const std::uint8_t> span) { setBytes(span); }
    explicit Mem64(const std::string& stringInput) { setString(stringInput); }

    // Basic info / views
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return m_buf.size();
    }
    [[nodiscard]] auto empty() const noexcept -> bool { return m_buf.empty(); }
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return m_buf.data();
    }
    [[nodiscard]] auto data() noexcept -> std::uint8_t* { return m_buf.data(); }

    // Read-only span view
    [[nodiscard]] auto bytes() const noexcept -> std::span<const std::uint8_t> {
        return {data(), size()};
    }

    // Assignment / write APIs
    void clear() { m_buf.clear(); }
    void reserve(std::size_t n) { m_buf.reserve(n); }

    void setBytes(const std::uint8_t* point, std::size_t n) {
        m_buf.assign(point, point + n);
    }
    void setBytes(std::span<const std::uint8_t> span) {
        setBytes(span.data(), span.size());
    }
    void setBytes(const std::vector<std::uint8_t>& val) { m_buf = val; }
    void setString(const std::string& stringInput) {
        const auto* point =
            std::bit_cast<const std::uint8_t*>(stringInput.data());
        setBytes(point, stringInput.size());
    }

    template <typename T>
    void setScalar(const T& val) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "setScalar requires trivially copyable T");
        m_buf.resize(sizeof(T));
        std::memcpy(m_buf.data(), &val, sizeof(T));
    }

    // Read APIs: interpret from start as host-endian scalar
    template <typename T>
    [[nodiscard]] auto tryGet() const noexcept -> std::optional<T> {
        if (m_buf.size() < sizeof(T)) {
            return std::nullopt;
        }
        T out{};
        std::memcpy(&out, m_buf.data(), sizeof(T));
        return out;
    }

    // Read API: non-optional variant (legacy migration), throws on failure
    template <typename T>
    [[nodiscard]] auto get() const -> T {
        auto opt = tryGet<T>();
        if (!opt) {
            throw std::runtime_error("Mem64::get<T>() insufficient bytes");
        }
        return *opt;
    }
};
