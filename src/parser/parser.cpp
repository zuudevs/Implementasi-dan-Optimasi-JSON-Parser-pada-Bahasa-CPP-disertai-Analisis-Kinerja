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

namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local std::vector<zuu::models::JsonValue> s_array_stack;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
thread_local std::vector<zuu::models::JsonMember> s_object_stack;
} // namespace

namespace zuu::parser {

Parser::Parser(Resources resources) noexcept
    : res_(resources.second), current_(resources.first.data()),
      end_(resources.first.data() + resources.first.size()) {
    s_array_stack.clear();
    s_object_stack.clear();

    const auto& meta = resources.second;
    const size_t hint = meta.comma_count + meta.array_count + meta.object_count;

    if (s_array_stack.capacity() < hint) {
        s_array_stack.reserve(hint);
    }

    if (s_object_stack.capacity() < hint) {
        s_object_stack.reserve(hint);
    }

    parse();
}

Parser::Expected Parser::Parse(Resources resources) noexcept {
    return Parser(resources).result();
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
    ++current_;
    return Parser::JsonValue::Integer(value);
}

Parser::JsonValue Parser::buildDouble() noexcept {
    double value{};
    const auto view = current_->value_;
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), value);

    if (ec != std::errc{} || ptr != view.data() + view.size()) {
        status_ = core::JsonError::InvalidValue;
        return models::JsonValue::Null();
    }
    current_++;
    return Parser::JsonValue::Double(value);
}

Parser::JsonValue Parser::buildString() noexcept {
    const auto index = res_.commitString(current_->value_);
    ++current_;
    return Parser::JsonValue::String(index);
}

Parser::JsonValue Parser::buildArray() noexcept {
    ++current_;

    if (current_->type_ == TokenType::RightSquareBracket) {
        ++current_;
        return JsonValue::Array(res_.commitArray(std::span<const JsonValue>{}));
    }

    const size_t start = s_array_stack.size();

    while (true) {
        auto value = buildValue();
        if (has_error()) [[unlikely]] {
            return JsonValue::Null();
        }

        s_array_stack.push_back(value);

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
            const size_t count = s_array_stack.size() - start;
            auto span = std::span<const models::JsonValue>(s_array_stack.data() + start, count);
            const auto idx = res_.commitArray(span);

            s_array_stack.resize(start);
            return models::JsonValue::Array(idx);
        }

        status_ = core::JsonError::MissingComma;
        return models::JsonValue::Null();
    }
}

Parser::JsonValue Parser::buildObject() noexcept {
    ++current_;

    if (current_->type_ == TokenType::RightCurlyBracket) {
        ++current_;
        return JsonValue::Object(res_.commitObject(std::span<const JsonMember>()));
    }

    const size_t start = s_object_stack.size();

    while (true) {
        if (current_->type_ != TokenType::String) [[unlikely]] {
            status_ = core::JsonError::UnquotedKey;
            return JsonValue::Null();
        }

        const auto key_index = res_.commitString(current_->value_);
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

        s_object_stack.push_back(JsonMember{.key_index_ = key_index, .value_ = value});

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
            const size_t count = s_object_stack.size() - start;
            auto span = std::span<const JsonMember>(s_object_stack.data() + start, count);
            const auto idx = res_.commitObject(span);

            s_object_stack.resize(start);
            return JsonValue::Object(idx);
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
    if (current_->type_ != TokenType::EndOfFile) {
        status_ = core::JsonError::InvalidValue;
    }
}

} // namespace zuu::parser