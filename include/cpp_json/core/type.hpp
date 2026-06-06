/**
 * @file type.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

namespace zuu::core {

enum class Type : unsigned char {
    Null,
    Boolean,
    Integer,
    Double,
    String,
    Array,
    Object,
};

} // namespace zuu::core