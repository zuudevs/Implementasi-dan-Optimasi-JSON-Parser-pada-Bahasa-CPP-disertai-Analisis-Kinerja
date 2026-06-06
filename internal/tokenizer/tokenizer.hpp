/**
 * @file tokenizer.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "cpp_json/core/error.hpp"
#include "models/token.hpp"
#include <expected>
#include <span>
#include <vector>

namespace zuu::tokenizer {

class Tokenizer {
  public:
    using Token = models::Token;
    using Error = core::JsonError;
	using Hint = models::Hint<Token>;
    using Result = std::vector<Token>;
	using Resources = std::pair<Result, Hint>;
    using Expected = std::expected<Resources, Error>;
    using Raw = std::span<const char>;

    explicit Tokenizer(Raw json_content) noexcept;
    Expected result() noexcept;

    [[nodiscard]] static Expected Tokenize(Tokenizer::Raw json_content) noexcept;

  private:
	models::Hint<Token> hint_;
    Result res_;
    const char* current_;
    const char* end_;
    Error status_{Error::None};

    [[nodiscard]] bool is_error() const noexcept;
    void readString() noexcept;
    void readNumeric() noexcept;
    void readAlphabet() noexcept;
    void tokenize() noexcept;
};

} // namespace zuu::tokenizer