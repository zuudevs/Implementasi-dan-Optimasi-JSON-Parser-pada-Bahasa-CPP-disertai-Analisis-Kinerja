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
#include "utils/strings.hpp"

namespace zuu::tokenizer {

Tokenizer::Tokenizer(std::span<const char> json_content) noexcept : raw_(json_content), hint_{} {
    res_.reserve(json_content.size() / 4);
    tokenize();
}

Tokenizer::Expected Tokenizer::result() noexcept {
    if (is_error()) {
        return std::unexpected{status_};
    }
    return Resources(std::move(res_), hint_);
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

void Tokenizer::skip_whitespace() noexcept {
    while (idx_ < raw_.size() && utils::is_whitespace(raw_[idx_])) {
        advance();
    }
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
    std::string_view keyword;
    Token::Type type{};

    switch (raw_[idx_]) {
        case 'n':
            keyword = "null";
            type = Token::Type::Null;
            break;
        case 't':
            keyword = "true";
            type = Token::Type::Boolean;
            break;
        case 'f':
            keyword = "false";
            type = Token::Type::Boolean;
            break;
        default:
            status_ = Error::InvalidValue;
            return;
    }

    if (idx_ + keyword.size() > raw_.size()) {
        status_ = Error::InvalidValue;
        return;
    }

    if (std::string_view(raw_.data() + idx_, keyword.size()) != keyword) {
        status_ = Error::InvalidValue;
        return;
    }

    size_t end = idx_ + keyword.size();
    if (end < raw_.size() && (utils::is_alphabet(raw_[end]) || utils::is_numeric(raw_[end]))) {
        status_ = Error::InvalidValue;
        return;
    }

    res_.emplace_back(type, std::string_view(raw_.data() + idx_, keyword.size()));

    idx_ = end;
}

void Tokenizer::tokenize() noexcept {
    while (idx_ < raw_.size()) {
        skip_whitespace();

        if (idx_ >= raw_.size()) {
            break;
        }

        char c = raw_[idx_];

        switch (c) {
            case '{': {
                res_.emplace_back(Token::Type::LeftCurlyBracket);
				hint_.object_count++;
                advance();
                continue;
            }
            case '}': {
                res_.emplace_back(Token::Type::RightCurlyBracket);
                advance();
                continue;
            }
            case '[': {
                res_.emplace_back(Token::Type::LeftSquareBracket);
				hint_.array_count++;
                advance();
                continue;
            }
            case ']': {
                res_.emplace_back(Token::Type::RightSquareBracket);
                advance();
                continue;
            }
            case ':': {
                res_.emplace_back(Token::Type::Colon);
				hint_.key_count++;
                advance();
                continue;
            }
            case ',': {
                res_.emplace_back(Token::Type::Comma);
                advance();
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
                if (utils::is_numeric(c) || c == '-') {
                    readNumeric();
                } else if (utils::is_alphabet(c)) {
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