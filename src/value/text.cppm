#pragma once
module;
#include <bit>
#include <codecvt>
#include <cstdint>
#include <cstring>
#include <locale>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module value.text;

import value.view;
import value.buffer;
import value.cursor;

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
                     const std::shared_ptr<ByteBuffer>& buffer) noexcept
        -> TextValue {
        const size_t START = buffer->size();
        size_t written = 0;
        switch (kind) {
            case TextKind::UTF8: {
                const uint8_t* bytes =
                    std::bit_cast<const uint8_t*>(text.data());
                written =
                    buffer->append(bytes, static_cast<size_t>(text.size()));
                break;
            }
            case TextKind::UTF16: {
                auto utf16 = toUtf16(text);  // UTF-8 -> UTF-16
                // copy to a transient byte vector to avoid aliasing issues
                std::vector<uint8_t> tmp;
                tmp.resize(utf16.size() * sizeof(char16_t));
                std::memcpy(tmp.data(), utf16.data(), tmp.size());
                written = buffer->append(tmp.data(), tmp.size());
                break;
            }
            case TextKind::UTF32: {
                auto utf32 = toUtf32(text);  // UTF-8 -> UTF-32
                std::vector<uint8_t> tmp;
                tmp.resize(utf32.size() * sizeof(char32_t));
                std::memcpy(tmp.data(), utf32.data(), tmp.size());
                written = buffer->append(tmp.data(), tmp.size());
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
        if (memView.size() == 0) {
            return std::nullopt;
        }
        return TextValue{.kind = kind, .view = memView};
    }

    // 获取字符串内容
    [[nodiscard]] auto get() const noexcept -> std::optional<std::string> {
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
    // 简单实现的 UTF 转换（基于 std::wstring_convert，C++20 已弃用但作为占位）
    static auto toUtf16(const std::string& utf8) -> std::u16string {
        std::u16string out;
        try {
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
                conv;
            out = conv.from_bytes(utf8);
        } catch (...) {
            out.clear();
        }
        return out;
    }

    static auto toUtf32(const std::string& utf8) -> std::u32string {
        std::u32string out;
        try {
            // 转到 UTF-32 通过先转到 UTF-16 再转换为 UTF-32
            auto tmp = toUtf16(utf8);
            out.assign(tmp.begin(), tmp.end());
        } catch (...) {
            out.clear();
        }
        return out;
    }

    static auto fromUtf16(const MemView& view) -> std::string {
        std::string out;
        try {
            // 拷贝为 char16_t 数组再转换
            size_t n16 = view.size() / sizeof(char16_t);
            std::vector<char16_t> tmp16(n16);
            std::memcpy(tmp16.data(), view.data(), n16 * sizeof(char16_t));
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
                conv;
            out = conv.to_bytes(tmp16.data(), tmp16.data() + n16);
        } catch (...) {
            out.clear();
        }
        return out;
    }

    static auto fromUtf32(const MemView& view) -> std::string {
        std::string out;
        try {
            // 拷贝为 char32_t 数组
            size_t n32 = view.size() / sizeof(char32_t);
            std::vector<char32_t> tmp32(n32);
            std::memcpy(tmp32.data(), view.data(), n32 * sizeof(char32_t));
            // 简单实现：直接把 char32_t 的 code points 转为 UTF-8
            for (size_t i = 0; i < n32; ++i) {
                char32_t codepoint = tmp32[i];
                if (codepoint <= 0x7F) {
                    out.push_back(static_cast<char>(codepoint));
                } else if (codepoint <= 0x7FF) {
                    out.push_back(
                        static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                } else if (codepoint <= 0xFFFF) {
                    out.push_back(
                        static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
                    out.push_back(
                        static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                } else {
                    out.push_back(
                        static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
                    out.push_back(
                        static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                    out.push_back(
                        static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                }
            }
        } catch (...) {
            out.clear();
        }
        return out;
    }
};