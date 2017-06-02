/* core.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// C++
#include <cstdint>
#include <cstdlib>
#include <type_traits>

// CAMY_INLINE
#define CAMY_INLINE __forceinline

// CAMY_API
#if defined(camy_dll)
#define camy_API __declspec(dllexport)
#else
#define CAMY_API
#endif

// CAMY_OS
#if defined(_WIN32)
#define CAMY_OS_WINDOWS
#else
#error Unsupported OS
#endif

// CAMY_ENABLE
// CAMY_ENABLE_LOGGING [n]
//		1 - Errors
//		2 - Errors & Warnings
//		3 - All
// CAMY_ENABLE_MEMORY_TRACKING
// CAMY_ENABLE_ASSERTS
#define CAMY_DEV
#if defined(CAMY_DEV)
#define CAMY_ENABLE_LOGGING 3
#define CAMY_ENABLE_MEMORY_TRACKING
#define CAMY_ENABLE_ASSERTS
#endif

// CAMY_BACKEND
#if !defined(CAMY_BACKEND_OPENGL4) && !defined(CAMY_BACKEND_D3D11)
#define CAMY_BACKEND_D3D11
#endif

namespace camy
{
    // Base types
    using uint8 = std::uint8_t;
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;
    using uint64 = std::uint64_t;

    using sint8 = std::int8_t;
    using sint16 = std::int16_t;
    using sint32 = std::int32_t;
    using sint64 = std::int64_t;

    using byte = uint8;
    using rsize = uint32;

    using char8 = char;
    using char16 = char16_t;

    namespace API
    {
        CAMY_INLINE rsize strlen(const char8* str);
        CAMY_INLINE bool strcmp(const char8* lhs, const char8* rhs);
        template <typename T>
        CAMY_INLINE T min(const T& a, const T& b);
        template <typename T>
        CAMY_INLINE T max(const T& a, const T& b);
        template <typename T>
        CAMY_INLINE bool is_power_of_two(T val);
    }

#define CAMY_ENUM_BITS(name, type)                                                                 \
    using name##Bits = type;                                                                       \
    enum CAMY_API name
}

#include "core.inl"