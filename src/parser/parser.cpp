/**
 * @file parser.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#include "parser/parser.hpp"
#include <charconv>

namespace zuu::parser {

Parser::Parser(Resources resources) noexcept
 : res_(resources.second)
 , current_(resources.first.data())
 , end_(resources.first.data() + resources.first.size()) {
    parse();
}

Parser::Expected Parser::result() const noexcept {
    if (has_error()) {
        return std::unexpected{status_};
    }
    return res_;
}

bool Parser::has_error() const noexcept {
    return status_ != Error::None;
}

Parser::Expected Parser::Parse(Resources resources) noexcept {
    return Parser(resources).result();
}

Parser::JsonValue Parser::buildNull() noexcept {
    current_++;
    return Parser::JsonValue::Null();
}

Parser::JsonValue Parser::buildBoolean() noexcept {
    const auto value = current_->value_ == "true";
    current_++;
    return Parser::JsonValue::Boolean(value);
}

Parser::JsonValue Parser::buildInteger() noexcept {
    long long value{};
    const auto view = current_->value_;
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), value);
    if (ec != std::errc{} || ptr != view.data() + view.size()) {
        status_ = core::JsonError::InvalidValue;
        return Parser::JsonValue::Null();
    }
    current_++;
    return Parser::JsonValue::Integer(value);
}

Parser::JsonValue Parser::buildDouble() noexcept {
    const auto view = current_->value_;
    std::string temp(view);
    char* end = nullptr;
    errno = 0;
    const long double value = std::strtold(temp.c_str(), &end);
    if (errno == ERANGE || end != temp.c_str() + temp.size()) {
        status_ = core::JsonError::InvalidValue;
        return Parser::JsonValue::Null();
    }
    current_++;
    return Parser::JsonValue::Double(value);
}

Parser::JsonValue Parser::buildString() noexcept {
    const auto index = res_.addString(current_->value_);
    current_++;
    return Parser::JsonValue::String(index);
}

Parser::JsonValue Parser::buildArray() noexcept {
    const auto array_index = res_.addArray();
    current_++;

    if (current_ >= end_) {
        status_ = core::JsonError::InvalidValue;
        return Parser::JsonValue::Null();
    }

    if (current_->type_ == TokenType::RightSquareBracket) {
        current_++;
        return Parser::JsonValue::Array(array_index);
    }

    while (current_ < end_) {
        if (current_->type_ == TokenType::RightSquareBracket || current_->type_ == TokenType::Comma) {
            status_ = core::JsonError::EmptyValue;
            return Parser::JsonValue::Null();
        }

        auto value = buildValue();
        if (has_error()) {
            return Parser::JsonValue::Null();
        }

        res_.array(array_index).push_back(value);

        if (current_ >= end_) {
            status_ = core::JsonError::InvalidValue;
            return Parser::JsonValue::Null();
        }

        if (current_->type_ == TokenType::Comma) {
            current_++;
            if (current_ < end_ && current_->type_ == TokenType::RightSquareBracket) {
                status_ = core::JsonError::TrailingComma;
                return Parser::JsonValue::Null();
            }
            continue;
        }

        if (current_->type_ == TokenType::RightSquareBracket) {
            current_++;
            return Parser::JsonValue::Array(array_index);
        }

        status_ = core::JsonError::MissingComma;
        return Parser::JsonValue::Null();
    }

    status_ = core::JsonError::InvalidValue;
    return Parser::JsonValue::Null();
}

Parser::JsonValue Parser::buildObject() noexcept {
    const auto object_index = res_.addObject();
    current_++;

    if (current_ >= end_) {
        status_ = core::JsonError::InvalidValue;
        return Parser::JsonValue::Null();
    }

    if (current_->type_ == TokenType::RightCurlyBracket) {
        current_++;
        return Parser::JsonValue::Object(object_index);
    }

    while (current_ < end_) {
        if (current_->type_ != TokenType::String) {
            status_ = core::JsonError::UnquotedKey;
            return Parser::JsonValue::Null();
        }

        const auto key_index = res_.addString(current_->value_);
        current_++;

        if (current_ >= end_ || current_->type_ != TokenType::Colon) {
            status_ = core::JsonError::InvalidType;
            return Parser::JsonValue::Null();
        }
        current_++;

        if (current_ >= end_) {
            status_ = core::JsonError::EmptyValue;
            return Parser::JsonValue::Null();
        }

        if (current_->type_ == TokenType::RightCurlyBracket || current_->type_ == TokenType::Comma) {
            status_ = core::JsonError::EmptyValue;
            return Parser::JsonValue::Null();
        }

        auto value = buildValue();
        if (has_error()) {
            return Parser::JsonValue::Null();
        }

        res_.object(object_index)
            .push_back(Parser::JsonMember{.key_index_ = key_index, .value_ = value});

        if (current_ >= end_) {
            status_ = core::JsonError::InvalidValue;
            return Parser::JsonValue::Null();
        }

        if (current_->type_ == TokenType::Comma) {
            current_++;
            if (current_ < end_ && current_->type_ == TokenType::RightCurlyBracket) {
                status_ = core::JsonError::TrailingComma;
                return Parser::JsonValue::Null();
            }
            continue;
        }

        if (current_->type_ == TokenType::RightCurlyBracket) {
            current_++;
            return Parser::JsonValue::Object(object_index);
        }

        status_ = core::JsonError::MissingComma;
        return Parser::JsonValue::Null();
    }

    status_ = core::JsonError::InvalidValue;
    return Parser::JsonValue::Null();
}

Parser::JsonValue Parser::buildValue() noexcept {
    if (current_ >= end_) {
        status_ = core::JsonError::EmptyValue;
        return Parser::JsonValue::Null();
    }

    switch (current_->type_) {
        case TokenType::Null:
            return buildNull();
        case TokenType::Boolean:
            return buildBoolean();
        case TokenType::Integer:
            return buildInteger();
        case TokenType::Double:
            return buildDouble();
        case TokenType::String:
            return buildString();
        case TokenType::LeftSquareBracket:
            return buildArray();
        case TokenType::LeftCurlyBracket:
            return buildObject();
        case TokenType::RightSquareBracket:
        case TokenType::RightCurlyBracket:
        case TokenType::Comma:
        case TokenType::Colon:
            status_ = core::JsonError::EmptyValue;
            return Parser::JsonValue::Null();
        default:
            status_ = core::JsonError::InvalidValue;
            return Parser::JsonValue::Null();
    }
}

void Parser::parse() noexcept {
    if (current_ == end_) {
        status_ = core::JsonError::EmptyValue;
        return;
    }

    auto root = buildValue();
    if (has_error()) {
        return;
    }

    res_.setRoot(root);
    if (current_ < end_) {
        status_ = core::JsonError::InvalidValue;
    }
}

} // namespace zuu::parser