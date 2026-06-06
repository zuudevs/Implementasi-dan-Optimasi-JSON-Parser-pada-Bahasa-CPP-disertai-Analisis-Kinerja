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

bool is_numeric(char c) {
    return c >= '0' && c <= '9';
}

bool is_alphabet(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_whitespace(char c) {
    return c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == ' ';
}

} // namespace zuu::utils