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
        LeftCurlyBracket, RightCurlyBracket,
        LeftSquareBracket, RightSquareBracket,
        Colon, Comma,
        Null, Boolean, Integer, Double, String,
        EndOfFile, Unknown,
    };

    Token(Type type, std::string_view value = "") noexcept
        : data_(value.data())
        , size_(static_cast<uint32_t>(value.size()))
        , type_(type) {}

    [[nodiscard]] constexpr std::string_view value() const noexcept {
        return {data_, size_};
    }

  private:
    const char* data_{nullptr};  // 8 bytes — offset  0
    uint32_t    size_{0};        // 4 bytes — offset  8
    Type        type_;           // 1 byte  — offset 12   ← was offset 16
    // 3 bytes padding
    // sizeof(Token) == 16  (was 24)

    friend class tokenizer::Tokenizer;
    friend class parser::Parser;
};

static_assert(sizeof(Token) == 16, "Token layout changed unexpectedly");

template <>
struct Hint<Token> {
	size_t string_count{0};
	size_t array_count{0};
	size_t object_count{0};
	size_t comma_count{0};
};

} // namespace models

} // namespace zuu