/* static_array.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/core.hpp>

#if CAMY_ENABLE_LOGGING > 0
#include <iostream>
#endif

/*
    Very simple small string implementation to be used mostly for naming and
    to avoid allocating from heap. Wrapped in bare-bone class that takes the space
    for the string itself + 4 bytes to cache the length. Can be reset.
    It's a useful alternative for when names are not hardcoded but generated
    and using std::string is overkill ( std::string should *not* be used )
    anywhere in the code. Except for parsing or loading operations ( possibly
    offline code ).
*/
namespace camy
{
    // Simple string implementation used for naming and some basic string building
    // operations. Basically an std::array with additional operations
    template <rsize t_char_count>
    class CAMY_API StaticString final
    {
    public:
        static const rsize CHAR_COUNT = t_char_count;

        using TStaticString = StaticString<CHAR_COUNT>;
        using Char = char8;
        using CharPtr = Char*;
        using CharConstPtr = const Char*;

    public:
        // Empty string
        StaticString();

        // Initializes from src string ( clamped if bigger than size )
        StaticString(CharConstPtr src);
        StaticString(CharConstPtr src, rsize len);
        StaticString(CharConstPtr start, CharConstPtr end);

        ~StaticString() = default;

        StaticString(const TStaticString& other);
        StaticString(TStaticString&& other);

        TStaticString& operator=(const TStaticString& other);
        TStaticString& operator=(TStaticString&& other);

        bool operator==(CharConstPtr str);

        // Equals against same size static string
        // For another static string w/o the same length
        // just use the raw-version ( might remove this one too in the future )
        bool equals(const TStaticString& other);

        // Equals against raw string
        bool equals(CharConstPtr other);

        // Returns a raw view of the string. It's perfectly safe to manipulate as needed.
        CharPtr str() { return m_buffer; }
        CharConstPtr str() const { return m_char_count == 0 ? nullptr : m_buffer; }

        void append(Char chr);
        void append(CharConstPtr str);
        void append(CharConstPtr start, CharConstPtr end);

        // Resets the string with a new src
        // if src == nullptr string will be empty
        void reset(CharConstPtr = nullptr);

        // Returns the lenth of the string ( or character count )
        rsize len() const { return m_char_count; }
        rsize capacity() const { return CHAR_COUNT; }
        bool empty() const { return m_char_count == 0; }

    private:
        void _append(CharConstPtr str);
        void _append_range(CharConstPtr start, CharConstPtr end);

        Char m_buffer[CHAR_COUNT + 1];
        rsize m_char_count;
    };

#if CAMY_ENABLE_LOGGING > 0
    // Overload for dumping to std::ostream
    template <rsize t_char_count>
    std::ostream& operator<<(std::ostream& os, const StaticString<t_char_count>& ss)
    {
        const char8* str = ss.str();
        if (str != nullptr)
            os << ss.str();
        return os;
    }
#endif

    template <rsize t_char_count>
    CAMY_INLINE StaticString<t_char_count>::StaticString()
        : m_char_count(0)
    {
        m_buffer[0] = (char8)'\0';
    }

    template <rsize t_char_count>
    CAMY_INLINE StaticString<t_char_count>::StaticString(CharConstPtr src)
    {
        m_char_count = 0;
        _append(src);
    }

    template <rsize t_char_count>
    CAMY_INLINE StaticString<t_char_count>::StaticString(CharConstPtr start, CharConstPtr end)
    {
        m_char_count = 0;
        _append_range(start, end);
    }

    template <rsize t_char_count>
    CAMY_INLINE StaticString<t_char_count>::StaticString(const TStaticString& other)
    {
        m_char_count = 0;
        _append(other.str());
    }

    template <rsize t_char_count>
    CAMY_INLINE StaticString<t_char_count>::StaticString(TStaticString&& other)
    {
        m_char_count = 0;
        _append(other.str());
    }

    template <rsize t_char_count>
    CAMY_INLINE typename StaticString<t_char_count>::TStaticString& StaticString<t_char_count>::
    operator=(const TStaticString& other)
    {
        _append(other.str());
        return *this;
    }

    template <rsize t_char_count>
    CAMY_INLINE typename StaticString<t_char_count>::TStaticString& StaticString<t_char_count>::
    operator=(TStaticString&& other)
    {
        _append(other.str());
        return *this;
    }

    template <rsize t_char_count>
    CAMY_INLINE bool StaticString<t_char_count>::operator==(CharConstPtr str)
    {
        return equals(str);
    }

    template <rsize t_char_count>
    CAMY_INLINE bool StaticString<t_char_count>::equals(const TStaticString& other)
    {
        return ::camy::strncmp(m_buffer, other.m_buffer, CHAR_COUNT);
    }

    template <rsize t_char_count>
    CAMY_INLINE bool StaticString<t_char_count>::equals(CharConstPtr other)
    {
        return strcmp(m_buffer, other) == 0;
    }

    template <rsize t_char_count>
    CAMY_INLINE void StaticString<t_char_count>::append(Char chr)
    {
        if (m_char_count < CHAR_COUNT - 1)
        {
            m_buffer[m_char_count] = chr;
            m_buffer[++m_char_count] = (char8)'\0';
        }
    }

    template <rsize t_char_count>
    CAMY_INLINE void StaticString<t_char_count>::append(CharConstPtr str)
    {
        _append(str);
    }

    template <rsize t_char_count>
    CAMY_INLINE void StaticString<t_char_count>::append(CharConstPtr start, CharConstPtr end)
    {
        _append_range(start, end);
    }

    template <rsize t_char_count>
    CAMY_INLINE void StaticString<t_char_count>::reset(CharConstPtr src)
    {
        m_char_count = 0;
        _append(src);
    }

    template <rsize t_char_count>
    CAMY_INLINE void StaticString<t_char_count>::_append(CharConstPtr str)
    {
        if (str == nullptr)
            return;

        rsize slen = API::min(API::strlen(str), CHAR_COUNT);
        memcpy(m_buffer + m_char_count, str, slen);
        m_char_count += slen;
        m_buffer[m_char_count] = 0;
    }

    template <rsize t_char_count>
    CAMY_INLINE void StaticString<t_char_count>::_append_range(CharConstPtr start, CharConstPtr end)
    {
        if (start == nullptr || end == nullptr || start == end)
            return;

        rsize slen = API::min((rsize)(end - start), CHAR_COUNT);
        memcpy(m_buffer + m_char_count, start, slen);
        m_char_count += slen;
        m_buffer[m_char_count] = 0;
    }
}