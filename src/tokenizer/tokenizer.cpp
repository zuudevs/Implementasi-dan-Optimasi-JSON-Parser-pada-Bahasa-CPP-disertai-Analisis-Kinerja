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
#include <cstring>

namespace zuu::tokenizer {

// Helper functions untuk operasi SWAR (SIMD Within A Register)
// Mencari byte bernilai 0x00 dalam data 64-bit (8 bytes) menggunakan bitwise algebra
[[nodiscard]] inline constexpr bool has_zero_byte(uint64_t v) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return (v - 0x0101010101010101ULL) & ~v & 0x8080808080808080ULL;
}

// Mengecek apakah terdapat karakter '"' (0x22) atau '\' (0x5C) di dalam 8 byte data
[[nodiscard]] inline constexpr bool has_quote_or_escape(uint64_t v) noexcept {
    const uint64_t quote_mask = v ^ 0x2222222222222222ULL;
    const uint64_t escape_mask = v ^ 0x5C5C5C5C5C5C5C5CULL;
    return has_zero_byte(quote_mask) | has_zero_byte(escape_mask);
}

Tokenizer::Tokenizer(std::span<const char> json_content) noexcept
    : current_(json_content.data()), end_(json_content.data() + json_content.size()) {
	// OPTIMIZE: Menggunakan pembagi 2 (bitshift right 1) + padding 16.
    // Menjamin 0 (nol) std::vector reallocation untuk high-density JSON.
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
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
    const char* begin = ++current_; // Lewati tanda kutip pembuka
    const char* ptr = begin;

    // Fast-path SWAR: Pindai 8 bytes sekaligus
    // Sangat brutal dan efisien untuk string berukuran sedang hingga besar
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    while (ptr + 8 <= end_) {
        uint64_t block{};
        // std::memcpy untuk unaligned memory read aman dan akan dioptimalkan 
        // oleh compiler LLVM/GCC menjadi instruksi load register tunggal (movq / ldr)
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        std::memcpy(&block, ptr, 8); 
        
        if (has_quote_or_escape(block)) {
            break; // Jika ada '"' atau '\', keluar dari fast-path dan serahkan ke slow-path
        }
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        ptr += 8;
    }

    // Slow-path: Evaluasi byte-by-byte untuk bagian ekor (sisa bytes) 
    // atau untuk evaluasi karakter escape '\'
    while (ptr < end_) {
        char c = *ptr;
        if (c == '"') [[likely]] {
            res_.emplace_back(Token::Type::String, std::string_view(begin, ptr - begin));
            current_ = ptr + 1;
            return;
        }
        if (c == '\\') [[unlikely]] {
            ptr += 2; // Lewati karakter escape
            if (ptr > end_) {
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
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        if (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
            status_ = core::JsonError::LeadingZero;
            return;
        }
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    } else if (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
        ++current_; // Telah dipastikan numerik, optimasi lewatkan digit pertama
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
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
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        if (current_ >= end_ || !(static_cast<unsigned>(*current_ - '0') < 10)) {
            status_ = core::JsonError::InvalidValue;
            return;
        }
        ++current_;
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
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

		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        if (current_ >= end_ || !(static_cast<unsigned>(*current_ - '0') < 10)) {
            status_ = core::JsonError::InvalidValue;
            return;
        }
        ++current_;
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        while (current_ < end_ && static_cast<unsigned>(*current_ - '0') < 10) {
            ++current_;
        }
    }

    // Eliminasi pengecekan is_error() redundan karena error memicu early-return.
    res_.emplace_back(type, std::string_view(begin, current_ - begin));
}

void Tokenizer::readAlphabet() noexcept {
    const auto rem = end_ - current_;
    char c = *current_;

    // Flattening: Menghindari jump table logic switch() untuk sekadar 3 key statis.
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    if (c == 'f' && rem >= 5 && current_[1] == 'a' && current_[2] == 'l' && current_[3] == 's' && current_[4] == 'e') {
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        res_.emplace_back(Token::Type::Boolean, std::string_view(current_, 5));
		// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
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
                // Skalar Optimization: Menggunakan local pointer (ptr) mengurangi write tracking overhead ke class state.
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