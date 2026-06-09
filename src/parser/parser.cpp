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
    : res_(resources.second), current_(resources.first.data()),
      end_(resources.first.data() + resources.first.size()) {
    parse();
}

Parser::Expected Parser::Parse(Resources resources) noexcept {
    return Parser(resources).result();
}

Parser::Expected Parser::result() && noexcept {
    if (has_error()) {
        return std::unexpected{status_};
    }
    return std::move(res_);
}

bool Parser::has_error() const noexcept {
    return status_ != Error::None;
}

Parser::JsonValue Parser::buildNull() noexcept {
    current_++;
    return Parser::JsonValue::Null();
}

Parser::JsonValue Parser::buildBoolean() noexcept {
    const auto value = (current_->value()[0] == 't');
    current_++;
    return Parser::JsonValue::Boolean(value);
}

Parser::JsonValue Parser::buildInteger() noexcept {
    const auto view = current_->value();
    const char* ptr = view.data();
    const char* end = ptr + view.size();
    
    long long value = 0;
    bool is_negative = false;

    if (*ptr == '-') {
        is_negative = true;
        ++ptr;
    }

    while (ptr < end) {
        value = (value << 3) + (value << 1) + (*ptr - '0'); 
        ++ptr;
    }

    if (is_negative) {
        value = -value;
    }

    ++current_;
    return Parser::JsonValue::Integer(value);
}

Parser::JsonValue Parser::buildDouble() noexcept {
    double value{};
    const auto view = current_->value();
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), value);

    if (ec != std::errc{} || ptr != view.data() + view.size()) {
        status_ = core::JsonError::InvalidValue;
        return models::JsonValue::Null();
    }
    current_++;
    return Parser::JsonValue::Double(value);
}

Parser::JsonValue Parser::buildString() noexcept {
    const auto index = res_.commitString(current_->value());
    ++current_;
    return Parser::JsonValue::String(index);
}

Parser::JsonValue Parser::buildArray() noexcept {
    ++current_;

    if (current_->type_ == TokenType::RightSquareBracket) {
        ++current_;
        return JsonValue::Array(res_.sealArray(res_.getArrayOffset()));
    }

    const size_t start_offset = res_.getArrayOffset();

    while (true) {
        auto value = buildValue();
        if (has_error()) [[unlikely]] {
            return JsonValue::Null();
        }

        res_.pushArrayElement(value);

        if (current_->type_ == TokenType::Comma) [[likely]] {
            ++current_;
            if (current_->type_ == TokenType::RightSquareBracket) [[unlikely]] {
                status_ = core::JsonError::TrailingComma;
                return models::JsonValue::Null();
            }
            continue;
        }

        if (current_->type_ == TokenType::RightSquareBracket) [[likely]] {
            ++current_;
            return models::JsonValue::Array(res_.sealArray(start_offset));
        }

        status_ = core::JsonError::MissingComma;
        return models::JsonValue::Null();
    }
}

Parser::JsonValue Parser::buildObject() noexcept {
    ++current_;

    if (current_->type_ == TokenType::RightCurlyBracket) {
        ++current_;
        return JsonValue::Object(res_.sealObject(res_.getObjectOffset()));
    }

    const size_t start_offset = res_.getObjectOffset();

    while (true) {
        if (current_->type_ != TokenType::String) [[unlikely]] {
            status_ = core::JsonError::UnquotedKey;
            return JsonValue::Null();
        }

        const auto key_index = res_.commitString(current_->value());
        ++current_;

        if (current_->type_ != TokenType::Colon) [[unlikely]] {
            status_ = core::JsonError::InvalidType;
            return JsonValue::Null();
        }
        ++current_;

        auto value = buildValue();
        if (has_error()) [[unlikely]] {
            return JsonValue::Null();
        }

        res_.pushObjectMember(JsonMember{.key_index_ = key_index, .value_ = value});

        if (current_->type_ == TokenType::Comma) [[likely]] {
            ++current_;
            if (current_->type_ == TokenType::RightCurlyBracket) [[unlikely]] {
                status_ = core::JsonError::TrailingComma;
                return JsonValue::Null();
            }
            continue;
        }

        if (current_->type_ == TokenType::RightCurlyBracket) [[likely]] {
            ++current_;
            return JsonValue::Object(res_.sealObject(start_offset));
        }

        status_ = core::JsonError::MissingComma;
        return JsonValue::Null();
    }
}

Parser::JsonValue Parser::buildValue() noexcept {
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
    if (current_ == end_ || current_->type_ == TokenType::EndOfFile) {
        status_ = core::JsonError::EmptyValue;
        return;
    }

    auto root = buildValue();
    if (has_error()) {
        return;
    }

    res_.setRoot(root);
    if (current_ == end_ || current_->type_ != TokenType::EndOfFile) {
        status_ = core::JsonError::InvalidValue;
    }
}

} // namespace zuu::parser