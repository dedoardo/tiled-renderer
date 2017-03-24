/* utils.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#include <camy/core/base.hpp>

namespace camy
{
    rsize strlen(const char8* str);
    void memcpy(void* dest, const void* src, rsize bytes);
    bool strncmp(const char8* a, const char8* b, rsize bytes);
    void strncpy(char8* dest, const char8* src, rsize bytes);
    bool strcmp(const char8* a, const char8* b);
    
    // Here just in case someone includes Windows w/o NOMINMAX
#	if defined(min)
#		undef min
#	endif
#	if defined(max)
#	    undef max
#	endif
    template <typename T> T min(const T& a, const T& b) { return a < b ? a : b; }
    template <typename T> T max(const T& a, const T& b) { return a < b ? b : a; }

    float  sin(float x);
    float  cos(float x);
    uint32 upper_pow2(uint32 value);
    uint32 bit_scan_reverse(uint32 value);

    template <typename ValType>
    inline bool is_power_of_two(ValType val)
    {
        static_assert(std::is_integral<ValType>(), "Power of two required integral");
        return (val != 0) && !(val & (val - 1));
    }
}