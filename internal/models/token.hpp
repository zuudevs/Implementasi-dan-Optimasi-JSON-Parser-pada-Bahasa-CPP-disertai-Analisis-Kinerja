/**
 * @file token.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "models/hint.hpp"
#include <string_view>

namespace zuu::models {

struct Token {
    enum class Type : unsigned char {
        LeftCurlyBracket,
        RightCurlyBracket,
        LeftSquareBracket,
        RightSquareBracket,
        Colon,
        Comma,
        Null,
        Boolean,
        Integer,
        Double,
        String,
		EndOfFile,
        Unknown,
    };

    Token(Type type, std::string_view value = "") noexcept : value_(value), type_(type) {}

    std::string_view value_;
    Type type_;
};

template <>
struct Hint<Token> {
	size_t string_count{0};
	size_t array_count{0};
	size_t object_count{0};
	size_t comma_count{0};
};

} // namespace zuu::models