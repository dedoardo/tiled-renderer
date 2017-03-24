/* utils.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/utils.hpp>

// libc
#include <cstring>

// intrin
#include <intrin.h>

namespace camy
{
    rsize strlen(const char8* str)
    {
        if (str == nullptr) return 0;
        return (rsize)std::strlen(str);
    }

    void memcpy(void* dest, const void* src, rsize bytes)
    {
        std::memcpy(dest, src, bytes);
    }

    void strncpy(char8 * dest, const char8 * src, rsize bytes)
    {
        std::strncpy(dest, src, bytes);
    }
    
    bool strncmp(const char8* a, const char8* b, rsize bytes)
    {
        return std::strncmp(a, b, bytes) == 0;
    }

    bool strcmp(const char8 * a, const char8 * b)
    {
        return std::strcmp(a, b) == 0;
    }

    float sin(float x)
    {
        return std::sinf(x);
    }

    float cos(float x)
    {
        return std::cosf(x);
    }

    uint32 upper_pow2(uint32 value)
    {
        unsigned long fbs{ 0 };
        _BitScanReverse(&fbs, value);

        // Now if value is say 16 we don't need to shift left
        // Note: not assuming overflow, we would need to decide whether 
        // to shift left by one or not, the problem is that the value 
        // returned by ~fbs & value is not 0 - 1 otherwise we could
        // just use it as argument for the shift. We could compute
        // the min/max w/o branching, but still, is overhead we don't really 
        // care about, so for now i'll go with branching
        return fbs + ((~(1 << fbs) & value) ? 1 : 0);
    }

    uint32 bit_scan_reverse(uint32 value)
    {
        unsigned long tz;
        _BitScanReverse(&tz, value);
        return tz;
    }
}