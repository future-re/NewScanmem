module;
#include <unicode/unistr.h>
#include <unicode/ustring.h>

#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module value.text;

import value.view;
import value.buffer;

export enum class TextKind : uint8_t { UTF8, UTF16, UTF32 };

export struct TextValue {
    TextKind kind{};
    MemView view;

    // 获取文本的字节大小
    [[nodiscard]] constexpr auto size() const noexcept -> size_t {
        return view.size();
    }

    // 将字符串写入 ByteBuffer 并生成 TextValue
    static auto make(const std::string& text, TextKind kind,
                     const std::shared_ptr<ByteBuffer>& buffer)
        -> TextValue {
        const size_t START = buffer->size();
        switch (kind) {
            case TextKind::UTF8: {
                const auto* bytes = std::bit_cast<const uint8_t*>(text.data());
                buffer->append(bytes, static_cast<size_t>(text.size()));
                break;
            }
            case TextKind::UTF16: {
                auto utf16 = toUtf16(text);  // UTF-8 -> UTF-16
                // copy to a transient byte vector to avoid aliasing issues
                std::vector<uint8_t> tmp;
                tmp.resize(utf16.size() * sizeof(char16_t));
                std::memcpy(tmp.data(), utf16.data(), tmp.size());
                buffer->append(tmp.data(), tmp.size());
                break;
            }
            case TextKind::UTF32: {
                auto utf32 = toUtf32(text);  // UTF-8 -> UTF-32
                std::vector<uint8_t> tmp;
                tmp.resize(utf32.size() * sizeof(char32_t));
                std::memcpy(tmp.data(), utf32.data(), tmp.size());
                buffer->append(tmp.data(), tmp.size());
                break;
            }
        }
        const size_t LEN = buffer->size() - START;
        return TextValue{.kind = kind,
                         .view = MemView::fromBuffer(buffer, START, LEN)};
    }

    // 从字节流中解析 TextValue
    static auto fromBytes(TextKind kind, MemView memView) noexcept
        -> std::optional<TextValue> {
        // 允许空字符串：直接返回 TextValue
        return TextValue{.kind = kind, .view = memView};
    }

    // 获取字符串内容
    [[nodiscard]] auto get() const -> std::optional<std::string> {
        switch (kind) {
            case TextKind::UTF8: {
                // 直接从字节拷贝到 std::string（避免 reinterpret_cast）
                const size_t N_VAL = view.size();
                std::string out;
                out.resize(N_VAL);
                std::memcpy(out.data(), view.data(), N_VAL);
                return out;
            }
            case TextKind::UTF16:
                return fromUtf16(view);  // UTF-16 -> UTF-8
            case TextKind::UTF32:
                return fromUtf32(view);  // UTF-32 -> UTF-8
        }
        return std::nullopt;
    }

   private:
    static auto toUtf16(const std::string& utf8) -> std::u16string {
        icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(utf8);

        // 先提取到 UChar 缓冲区，避免将 char16_t* 当作 UChar*
        int32_t cap16 = unicodeStr.length();
        if (cap16 == 0) {
            return {};
        }
        std::vector<UChar> u16(static_cast<size_t>(cap16));
        UErrorCode err = U_ZERO_ERROR;
        int32_t actualLen = unicodeStr.extract(u16.data(), cap16, err);
        if (U_FAILURE(err) != 0) {
            return {};
        }
        // 拷贝到 std::u16string
        std::u16string utf16(static_cast<size_t>(actualLen), u'\0');
        std::memcpy(utf16.data(), u16.data(), static_cast<size_t>(actualLen) * sizeof(UChar));
        return utf16;
    }

    static auto toUtf32(const std::string& utf8) -> std::u32string {
        icu::UnicodeString unicodeStr = icu::UnicodeString::fromUTF8(utf8);

        // 第一步：提取 UTF-16 (UChar)
        int32_t cap16 = unicodeStr.length();
        if (cap16 == 0) {
            return {};
        }
        std::vector<UChar> u16(static_cast<size_t>(cap16));
        UErrorCode err = U_ZERO_ERROR;
        int32_t len16 = unicodeStr.extract(u16.data(), cap16, err);
        if (U_FAILURE(err) != 0) {
            return {};
        }

        // 第二步：UTF-16 -> UTF-32 (UChar32)，两段式获取长度
        int32_t len32 = 0;
        err = U_ZERO_ERROR;
        u_strToUTF32(nullptr, 0, &len32, u16.data(), len16, &err);
        if (err != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(err) != 0) {
            return {};
        }
        err = U_ZERO_ERROR;
        std::vector<UChar32> u32(static_cast<size_t>(len32));
        u_strToUTF32(u32.data(), len32, &len32, u16.data(), len16, &err);
        if (U_FAILURE(err) != 0) {
            return {};
        }

        // 拷贝到 std::u32string
        std::u32string utf32(static_cast<size_t>(len32), U'\0');
        std::memcpy(utf32.data(), u32.data(), static_cast<size_t>(len32) * sizeof(UChar32));
        return utf32;
    }

    static auto fromUtf16(const MemView& view) -> std::string {
        // 以字节形式拷贝到 UChar 缓冲区，避免别名问题
        size_t n16 = view.size() / sizeof(char16_t);
        std::vector<UChar> u16(n16);
        std::memcpy(u16.data(), view.data(), n16 * sizeof(UChar));
        // ICU: 构造 UnicodeString 并转 UTF-8
        icu::UnicodeString unicodeStr(u16.data(), static_cast<int32_t>(n16));
        std::string out;
        unicodeStr.toUTF8String(out);
        return out;
    }

    static auto fromUtf32(const MemView& view) -> std::string {
        // 拷贝为 UChar32 数组
        size_t n32 = view.size() / sizeof(char32_t);
        std::vector<UChar32> u32(n32);
        std::memcpy(u32.data(), view.data(), n32 * sizeof(UChar32));

        // 先计算 UTF-16 长度，再分配并转换
        UErrorCode err = U_ZERO_ERROR;
        int32_t utf16Len = 0;
        u_strFromUTF32(nullptr, 0, &utf16Len, u32.data(), static_cast<int32_t>(n32), &err);
        if (err != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(err) != 0) {
            return {};
        }
        err = U_ZERO_ERROR;
        std::vector<UChar> utf16(static_cast<size_t>(utf16Len));
        u_strFromUTF32(utf16.data(), utf16Len, &utf16Len, u32.data(), static_cast<int32_t>(n32), &err);
        if (U_FAILURE(err) != 0) {
            return {};
        }

        // 用 UnicodeString 构造，再转 UTF-8
        icu::UnicodeString unicodeStr(utf16.data(), utf16Len);
        std::string out;
        unicodeStr.toUTF8String(out);
        return out;
    }
};
