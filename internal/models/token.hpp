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

namespace zuu {

namespace tokenizer {

class Tokenizer;

} // namespace tokenizer

namespace parser {

class Parser;

} // namespace parser

namespace models {

class Token {
  public:
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

  private:
    std::string_view value_;
    Type type_;

    friend class tokenizer::Tokenizer;
    friend class parser::Parser;
};

template <>
struct Hint<Token> {
	size_t string_count{0};
	size_t array_count{0};
	size_t object_count{0};
	size_t comma_count{0};
};

} // namespace models

} // namespace zuu