/* utils.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#include <camy/core/base.hpp>

// libc
#include <string.h>

namespace camy
{
	// Slightly safer version of strlen that returns 0 when string is nullptr
	camy_inline rsize s_strlen(const char8* str)
	{
		if (str == nullptr)
			return (rsize)0;

		return (rsize)::strlen(str);
	}

	camy_inline bool s_strcmp(const char8* a, const char8* b)
	{
		return ::strcmp(a, b) == 0;
	}
    
    // Here just in case someone includes Windows w/o NOMINMAX
#	if defined(min)
#		undef min
#	endif
#	if defined(max)
#	    undef max
#	endif
    template <typename T> 
	camy_inline T min(const T& a, const T& b) { return a < b ? a : b; }
    
	template <typename T> 
	camy_inline T max(const T& a, const T& b) { return a < b ? b : a; }
	
	template <typename T>
	camy_inline bool is_power_of_two(T val)
	{
		static_assert(std::is_integral<T>(), "Power of two required integral");
		return (val != 0) && !(val & (val - 1));
	}
}