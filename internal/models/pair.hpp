/**
 * @file pair.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-06-06
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

namespace zuu::models {

template <typename First, typename Second>
struct Pair {
	First first;
	Second second;
};

template <typename First, typename Second>
Pair(First, Second) -> Pair<First, Second>;

} // namespace zuu::models