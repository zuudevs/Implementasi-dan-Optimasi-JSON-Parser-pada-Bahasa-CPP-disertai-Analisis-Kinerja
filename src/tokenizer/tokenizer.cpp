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

Tokenizer::Tokenizer(std::span<const char> json_content) noexcept : raw_(json_content) {
    res_.reserve(json_content.size() / 4);
    tokenize();
}

Tokenizer::Expected Tokenizer::result() noexcept {
    if (is_error()) {
        return std::unexpected{status_};
    }
    return res_;
}

Tokenizer::Expected Tokenizer::Tokenize(Tokenizer::Raw json_content) noexcept {
    auto tokens = Tokenizer(json_content);
    return tokens.result();
}

bool Tokenizer::is_error() const noexcept {
    return status_ != Error::None;
}

void Tokenizer::advance() noexcept {
    idx_++;
}

void Tokenizer::readString() noexcept {
    advance();
    size_t start = idx_;

    while (idx_ < raw_.size()) {
        if (raw_[idx_] == '\\') {
            advance();
            if (idx_ < raw_.size()) {
                advance();
            }
            continue;
        }
        if (raw_[idx_] == '\"') {
            break;
        }
        advance();
    }

    if (idx_ >= raw_.size() || raw_[idx_] != '\"') {
        status_ = Error::InvalidValue;
        return;
    }

    res_.emplace_back(Token::Type::String, std::string_view(raw_.data() + start, idx_ - start));

    advance();
}

void Tokenizer::readNumeric() noexcept {
    size_t start = idx_;
    auto type = Token::Type::Integer;

    if (idx_ < raw_.size() && raw_[idx_] == '-') {
        advance();
    }

    if (idx_ < raw_.size() && raw_[idx_] == '0') {
        advance();
        if (idx_ < raw_.size() && utils::is_numeric(raw_[idx_])) {
            status_ = Error::LeadingZero;
            return;
        }
    } else if (idx_ < raw_.size() && utils::is_numeric(raw_[idx_])) {
        while (idx_ < raw_.size() && utils::is_numeric(raw_[idx_])) {
            advance();
        }
    } else {
        status_ = Error::InvalidValue;
        return;
    }

    if (idx_ < raw_.size() && raw_[idx_] == '.') {
        type = Token::Type::Double;
        advance();

        if (idx_ >= raw_.size() || !utils::is_numeric(raw_[idx_])) {
            status_ = Error::InvalidValue;
            return;
        }

        while (idx_ < raw_.size() && utils::is_numeric(raw_[idx_])) {
            advance();
        }
    }

    if (idx_ < raw_.size() && (raw_[idx_] == 'e' || raw_[idx_] == 'E')) {
        type = Token::Type::Double;
        advance();

        if (idx_ < raw_.size() && (raw_[idx_] == '+' || raw_[idx_] == '-')) {
            advance();
        }

        if (idx_ >= raw_.size() || !utils::is_numeric(raw_[idx_])) {
            status_ = Error::InvalidValue;
            return;
        }

        while (idx_ < raw_.size() && utils::is_numeric(raw_[idx_])) {
            advance();
        }
    }

    if (!is_error()) {
        res_.emplace_back(type, std::string_view(raw_.data() + start, idx_ - start));
    }
}

void Tokenizer::readAlphabet() noexcept {
    switch (raw_[idx_]) {
        case 'n':
            if (idx_ + 4 <= raw_.size() && memcmp(raw_.data() + idx_, "null", 4) == 0) {
                res_.emplace_back(Token::Type::Null, std::string_view(raw_.data() + idx_, 4));

                idx_ += 4;
                return;
            }
            break;

        case 't':
            if (idx_ + 4 <= raw_.size() && memcmp(raw_.data() + idx_, "true", 4) == 0) {
                res_.emplace_back(Token::Type::Boolean, std::string_view(raw_.data() + idx_, 4));

                idx_ += 4;
                return;
            }
            break;

        case 'f':
            if (idx_ + 5 <= raw_.size() && memcmp(raw_.data() + idx_, "false", 5) == 0) {
                res_.emplace_back(Token::Type::Boolean, std::string_view(raw_.data() + idx_, 5));

                idx_ += 5;
                return;
            }
            break;
		default:
			status_ = Error::InvalidValue;
    }
}

void Tokenizer::tokenize() noexcept {
    while (idx_ < raw_.size()) {
        switch (constants::LUT_TOKEN[raw_[idx_]]) {
			case 0: {
				while (idx_ < raw_.size() && utils::is_whitespace(raw_[idx_])) {
					advance();
				}
				continue;
			}
            case 1: {
                res_.emplace_back(Token::Type::LeftCurlyBracket);
                advance();
                continue;
            }
            case 2: {
                res_.emplace_back(Token::Type::RightCurlyBracket);
                advance();
                continue;
            }
            case 3: {
                res_.emplace_back(Token::Type::LeftSquareBracket);
                advance();
                continue;
            }
            case 4: {
                res_.emplace_back(Token::Type::RightSquareBracket);
                advance();
                continue;
            }
            case 5: {
                res_.emplace_back(Token::Type::Colon);
                advance();
                continue;
            }
            case 6: {
                res_.emplace_back(Token::Type::Comma);
                advance();
                continue;
            }
            case 7: {
                readString();
                if (is_error())
                    return;
                continue;
            }
            case 8: {
                readNumeric();
                if (is_error())
                    return;
                continue;
            }
            case 9: {
                readAlphabet();
                if (is_error())
                    return;
                continue;
            }
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
}

} // namespace zuu::tokenizer