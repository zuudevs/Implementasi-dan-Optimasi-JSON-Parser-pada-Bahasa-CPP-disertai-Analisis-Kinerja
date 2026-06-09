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
#include "utils/strings.hpp"

namespace zuu::tokenizer {

Tokenizer::Tokenizer(std::span<const char> json_content) noexcept
    : current_(json_content.data()), end_(json_content.data() + json_content.size()) {
    res_.reserve(json_content.size() / 3 + 4);
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

    while (current_ < end_) {
        const char* next_quote =
            static_cast<const char*>(memchr(current_, '"', static_cast<size_t>(end_ - current_)));

        if (!next_quote) [[unlikely]] {
            status_ = core::JsonError::InvalidValue;
            return;
        }

        const char* next_bs = static_cast<const char*>(
            memchr(current_, '\\', static_cast<size_t>(next_quote - current_)));

        if (!next_bs) [[likely]] {
            res_.emplace_back(Token::Type::String, std::string_view(begin, next_quote - begin));
            current_ = next_quote + 1;
            return;
        }

        current_ = next_bs + 2;
        if (current_ > end_) [[unlikely]] {
            status_ = core::JsonError::InvalidValue;
            return;
        }
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
        if (current_ < end_ && utils::is_numeric(*current_)) {
            status_ = core::JsonError::LeadingZero;
            return;
        }
    } else if (current_ < end_ && utils::is_numeric(*current_)) {
        while (current_ < end_ && utils::is_numeric(*current_)) {
            ++current_;
        }
    } else {
        status_ = core::JsonError::InvalidValue;
        return;
    }

    if (current_ < end_ && *current_ == '.') {
        type = Token::Type::Double;
        ++current_;
        if (current_ >= end_ || !utils::is_numeric(*current_)) {
            status_ = core::JsonError::InvalidValue;
            return;
        }

        while (current_ < end_ && utils::is_numeric(*current_)) {
            ++current_;
        }
    }

    if (current_ < end_ && (*current_ == 'e' || *current_ == 'E')) {
        type = Token::Type::Double;
        ++current_;
        if (current_ < end_ && (*current_ == '+' || *current_ == '-')) {
            ++current_;
        }

        if (current_ >= end_ || !utils::is_numeric(*current_)) {
            status_ = core::JsonError::InvalidValue;
            return;
        }

        while (current_ < end_ && utils::is_numeric(*current_)) {
            ++current_;
        }
    }

    if (!is_error()) {
        res_.emplace_back(type, std::string_view(begin, current_ - begin));
    }
}

void Tokenizer::readAlphabet() noexcept {
    switch (*current_) {
        case 'n': {
            constexpr auto size = sizeof("null") - 1;
            if (current_ + size <= end_ && memcmp(current_ + 1, "ull", size - 1) == 0) {
                res_.emplace_back(Token::Type::Null, std::string_view(current_, size));
                current_ += size;
                return;
            }
            break;
        }
        case 't': {
            constexpr auto size = sizeof("true") - 1;
            if (current_ + size <= end_ && memcmp(current_ + 1, "rue", size - 1) == 0) {
                res_.emplace_back(Token::Type::Boolean, std::string_view(current_, size));

                current_ += size;
                return;
            }
            break;
        }
        case 'f': {
            constexpr auto size = sizeof("false") - 1;
            if (current_ + size <= end_ && memcmp(current_ + 1, "alse", size - 1) == 0) {
                res_.emplace_back(Token::Type::Boolean, std::string_view(current_, size));

                current_ += size;
                return;
            }
            break;
        }
        default:
            break;
    }
    status_ = Error::InvalidValue;
}

void Tokenizer::tokenize() noexcept {
    while (current_ < end_) {
		// NOLINENEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        switch (*(constants::LUT_TOKEN + *current_)) {
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            case 0: {
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
                do {
                    ++current_;
                } while (current_ < end_ &&
                         constants::LUT_TOKEN[static_cast<unsigned char>(*current_)] == 0);
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