/**
 * @file strings.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-01
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

namespace zuu::utils {

[[nodiscard]] inline constexpr bool is_numeric(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] inline constexpr bool is_alphabet(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

[[nodiscard]] inline constexpr bool is_whitespace(char c) noexcept {
    return c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == ' ';
}

} // namespace zuu::utils