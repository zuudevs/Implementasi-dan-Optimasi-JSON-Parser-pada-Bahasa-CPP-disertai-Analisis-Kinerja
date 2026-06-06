/**
 * @file tokenizer.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#include "constants/literal.hpp"
#include "tokenizer/tokenizer.hpp"
#include "utils/strings.hpp"

namespace zuu::tokenizer {

Tokenizer::Tokenizer(std::span<const char> json_content) noexcept
 : current_(json_content.data())
 , end_(json_content.data() + json_content.size()) {
    res_.reserve(json_content.size() / 4);
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

void Tokenizer::skip_whitespace() noexcept {
    while (current_ < end_ && utils::is_whitespace(*current_)) {
        current_++;
    }
}

void Tokenizer::readString() noexcept {
    auto begin = ++current_;

    while (current_ < end_) {
        if (*current_ == '\\') {
            current_++;
            if (current_ < end_) {
                current_++;
            }
            continue;
        }
        if (*current_ == '\"') {
            break;
        }
        current_++;
    }

    if (current_ >= end_ || *current_ != '\"') {
        status_ = Error::InvalidValue;
        return;
    }

    res_.emplace_back(Token::Type::String, std::string_view(begin, current_ - begin));
    current_++;
}

void Tokenizer::readNumeric() noexcept {
    auto begin = current_;
    auto type = Token::Type::Integer;

    if (current_ < end_ && *current_ == '-') {
        current_++;
    }

    if (current_ < end_ && *current_ == '0') {
        current_++;
        if (current_ < end_ && utils::is_numeric(*current_)) {
            status_ = Error::LeadingZero;
            return;
        }
    } else if (current_ < end_ && utils::is_numeric(*current_)) {
        while (current_ < end_ && utils::is_numeric(*current_)) {
            current_++;
        }
    } else {
        status_ = Error::InvalidValue;
        return;
    }

    if (current_ < end_ && *current_ == '.') {
        type = Token::Type::Double;
        current_++;

        if (current_ >= end_ || !utils::is_numeric(*current_)) {
            status_ = Error::InvalidValue;
            return;
        }

        while (current_ < end_ && utils::is_numeric(*current_)) {
            current_++;
        }
    }

    if (current_ < end_ && (*current_ == 'e' || *current_ == 'E')) {
        type = Token::Type::Double;
        current_++;

        if (current_ < end_ && (*current_ == '+' || *current_ == '-')) {
            current_++;
        }

        if (current_ >= end_ || !utils::is_numeric(*current_)) {
            status_ = Error::InvalidValue;
            return;
        }

        while (current_ < end_ && utils::is_numeric(*current_)) {
            current_++;
        }
    }

    if (!is_error()) {
        res_.emplace_back(type, std::string_view(begin, current_ - begin));
    }
}

void Tokenizer::readAlphabet() noexcept {
    unsigned char keyword_idx{0};
    Token::Type type{};

    switch (*current_) {
        case 'n':
            keyword_idx = 0;
            type = Token::Type::Null;
            break;
        case 't':
            keyword_idx = 1;
            type = Token::Type::Boolean;
            break;
        case 'f':
            keyword_idx = 2;
            type = Token::Type::Boolean;
            break;
        default:
            status_ = Error::InvalidValue;
            return;
    }

    if (current_ + constants::JSON_LIT_SIZE[keyword_idx] > end_) {
        status_ = Error::InvalidValue;
        return;
    }

    if (!std::equal(
		constants::JSON_LIT[keyword_idx],
		constants::JSON_LIT[keyword_idx] + constants::JSON_LIT_SIZE[keyword_idx],
		current_
	)) {
		status_ = core::JsonError::InvalidValue;
		return;
	}

	auto end_lit = current_ + constants::JSON_LIT_SIZE[keyword_idx];
	if (end_lit < end_ && (utils::is_alphabet(*end_lit) || utils::is_numeric(*end_lit))) {
		status_ = core::JsonError::InvalidValue;
		return;
	}

	res_.emplace_back(type, std::string_view(current_, end_lit - current_));
	current_ = end_lit;
}

void Tokenizer::tokenize() noexcept {
    while (current_ < end_) {
        skip_whitespace();

        if (current_ >= end_) {
            break;
        }

        switch (*current_) {
            case '{': {
                res_.emplace_back(Token::Type::LeftCurlyBracket);
				hint_.object_count++;
                current_++;
                continue;
            }
            case '}': {
                res_.emplace_back(Token::Type::RightCurlyBracket);
                current_++;
                continue;
            }
            case '[': {
                res_.emplace_back(Token::Type::LeftSquareBracket);
				hint_.array_count++;
                current_++;
                continue;
            }
            case ']': {
                res_.emplace_back(Token::Type::RightSquareBracket);
                current_++;
                continue;
            }
            case ':': {
                res_.emplace_back(Token::Type::Colon);
				hint_.key_count++;
                current_++;
                continue;
            }
            case ',': {
                res_.emplace_back(Token::Type::Comma);
                current_++;
                continue;
            }
            case '\"': {
                readString();
                if (is_error())
                    return;
				hint_.string_count++;
                continue;
            }
            case '\'': {
                status_ = Error::SingleQuotedString;
                return;
            }
            default: {
                if (utils::is_numeric(*current_) || *current_ == '-') {
                    readNumeric();
                } else if (utils::is_alphabet(*current_)) {
                    readAlphabet();
                } else {
                    status_ = Error::Unknown;
                    return;
                }

                if (is_error()) {
                    return;
                }
            }
        }
    }
}

} // namespace zuu::tokenizer