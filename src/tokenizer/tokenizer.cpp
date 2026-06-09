/**
 * @file tokenizer.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#include "tokenizer/tokenizer.hpp"
#include "constants/token_lut.hpp"

namespace zuu::tokenizer {

// ── SWAR (SIMD Within A Register) Helpers ────────────────────────────────────

[[nodiscard]] inline constexpr bool has_zero_byte(uint64_t v) noexcept {
    return (v - 0x0101010101010101ULL) & ~v & 0x8080808080808080ULL;
}

[[nodiscard]] inline constexpr bool has_quote_or_escape(uint64_t v) noexcept {
    const uint64_t quote_mask = v ^ 0x2222222222222222ULL;
    const uint64_t escape_mask = v ^ 0x5C5C5C5C5C5C5C5CULL;
    return has_zero_byte(quote_mask) | has_zero_byte(escape_mask);
}

// ─────────────────────────────────────────────────────────────────────────────

Tokenizer::Tokenizer(std::span<const char> json_content) noexcept
    : current_(json_content.data())
    , end_(json_content.data() + json_content.size()) {
    res_.reserve((json_content.size() >> 1) + 16);
    tokenize();
}

Tokenizer::Expected Tokenizer::result() noexcept {
    if (is_error()) {
        return std::unexpected{status_};
    }
    return std::pair{std::move(res_), hint_};
}

Tokenizer::Expected Tokenizer::Tokenize(Tokenizer::Raw json_content) noexcept {
    auto tokens = Tokenizer(json_content);
    return tokens.result();
}

bool Tokenizer::is_error() const noexcept {
    return status_ != Error::None;
}

void Tokenizer::readString() noexcept {
    const char* begin = ++current_;
    const char* ptr = begin;
    const char* end = end_;
    bool has_escape = false;

    // Fast-path SWAR: Pindai 8 bytes sekaligus
    while (ptr + 8 <= end) {
        uint64_t block{};
        std::memcpy(&block, ptr, 8);

        if (has_quote_or_escape(block)) {
            break; // Jika ada '"' atau '\', keluar dan evaluasi secara presisi di slow-path
        }
        ptr += 8;
    }

    // Slow-path skalar untuk resolusi akhir dan tracking escape character
    while (ptr < end) {
        char c = *ptr;
        if (c == '"') [[likely]] {
            res_.emplace_back(
                Token::Type::String, std::string_view(begin, ptr - begin), has_escape);

            // Catat kebutuhan buffer jika string memiliki escape
            if (has_escape) {
                hint_.string_escape_bytes += (ptr - begin);
            }

            current_ = ptr + 1;
            return;
        }
        if (c == '\\') [[unlikely]] {
            has_escape = true;
            ptr += 2; // Lewati karakter escape
            if (ptr > end) {
                status_ = core::JsonError::InvalidValue;
                return;
            }
            continue;
        }
        ++ptr;
    }

    status_ = core::JsonError::InvalidValue;
}

void Tokenizer::readNumeric() noexcept {
    const char* begin = current_;
    auto type = Token::Type::Integer;

    if (current_ < end_ && *current_ == '-') {
        ++current_;
    }

    if (current_ < end_ && *current_ == '0') {
        ++current_;
        if (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
            status_ = core::JsonError::LeadingZero;
            return;
        }
    } else if (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
        ++current_;
        while (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
            ++current_;
        }
    } else {
        status_ = core::JsonError::InvalidValue;
        return;
    }

    if (current_ < end_ && *current_ == '.') {
        type = Token::Type::Double;
        ++current_;
        if (current_ >= end_ || !(static_cast<unsigned>(*current_ - '0') < 10)) {
            status_ = core::JsonError::InvalidValue;
            return;
        }
        ++current_;
        while (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
            ++current_;
        }
    }

    if (current_ < end_ && (*current_ == 'e' || *current_ == 'E')) {
        type = Token::Type::Double;
        ++current_;
        if (current_ < end_ && (*current_ == '+' || *current_ == '-')) {
            ++current_;
        }

        if (current_ >= end_ || !(static_cast<unsigned>(*current_ - '0') < 10)) {
            status_ = core::JsonError::InvalidValue;
            return;
        }
        ++current_;
        while (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
            ++current_;
        }
    }

    res_.emplace_back(type, std::string_view(begin, current_ - begin));
}

void Tokenizer::readAlphabet() noexcept {
    const auto rem = end_ - current_;
    char c = *current_;

    if (c == 'f' && rem >= 5 && current_[1] == 'a' && current_[2] == 'l' && current_[3] == 's' &&
        current_[4] == 'e') {
        res_.emplace_back(Token::Type::Boolean, std::string_view(current_, 5));
        current_ += 5;
        return;
    }
    if (c == 't' && rem >= 4 && current_[1] == 'r' && current_[2] == 'u' && current_[3] == 'e') {
        res_.emplace_back(Token::Type::Boolean, std::string_view(current_, 4));
        current_ += 4;
        return;
    }
    if (c == 'n' && rem >= 4 && current_[1] == 'u' && current_[2] == 'l' && current_[3] == 'l') {
        res_.emplace_back(Token::Type::Null, std::string_view(current_, 4));
        current_ += 4;
        return;
    }

    status_ = Error::InvalidValue;
}

void Tokenizer::tokenize() noexcept {
    while (current_ < end_) {
        switch (constants::LUT_TOKEN[static_cast<unsigned char>(*current_)]) {
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 0: {
                const char* ptr = current_ + 1;
                while (ptr < end_ && constants::LUT_TOKEN[static_cast<unsigned char>(*ptr)] == 0) {
                    ++ptr;
                }
                current_ = ptr;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 1: {
                res_.emplace_back(Token::Type::LeftCurlyBracket);
                hint_.object_count++;
                current_++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 2: {
                res_.emplace_back(Token::Type::RightCurlyBracket);
                current_++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 3: {
                res_.emplace_back(Token::Type::LeftSquareBracket);
                hint_.array_count++;
                current_++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 4: {
                res_.emplace_back(Token::Type::RightSquareBracket);
                current_++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 5: {
                res_.emplace_back(Token::Type::Colon);
                current_++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 6: {
                res_.emplace_back(Token::Type::Comma);
                hint_.comma_count++;
                current_++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 7: {
                readString();
                if (is_error())
                    return;
                hint_.string_count++;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 8: {
                readNumeric();
                if (is_error())
                    return;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 9: {
                readAlphabet();
                if (is_error())
                    return;
                continue;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 10: {
                status_ = Error::SingleQuotedString;
                return;
            }
            default: {
                status_ = Error::Unknown;
                return;
            }
        }
    }
    res_.emplace_back(Token::Type::EndOfFile);
}

} // namespace zuu::tokenizer