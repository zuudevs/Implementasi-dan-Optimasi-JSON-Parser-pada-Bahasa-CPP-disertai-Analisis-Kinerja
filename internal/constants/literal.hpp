/**
 * @file literal.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

namespace zuu::constants {

inline constexpr const char* JSON_LIT[3] = {
	"null",
	"true",
	"false"
};

inline constexpr unsigned char JSON_LIT_SIZE[3] = {
	4,
	4,
	5
};

} // namespace zuu::constants