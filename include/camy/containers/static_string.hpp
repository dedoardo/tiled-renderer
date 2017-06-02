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

		Strictly ASCII for now
*/
namespace camy
{
	template <rsize kCharCount = 11>
	class CAMY_API StaticString final
	{
	public:
		// Empty name
		StaticString();

		// Initializes from src string ( clamped if bigger than size )
		StaticString(const char8* src);
		StaticString(const char8* src, rsize len);
		StaticString(const char8* start, const char8* end);

		// No deallocation happens
		~StaticString() = default;

		StaticString(const StaticString& other);
		StaticString<kCharCount>& operator=(const StaticString& other);

		bool operator==(const char8* str);
		// Equals against same size static string
		// For another static string w/o the same length
		// just use the raw-version ( might remove this one too in the future )
		bool equals(const StaticString<kCharCount>& other);

		// Equals against raw string
		bool equals(const char8* other);

		// Returns a raw view of the string
		char8* str() { return m_buffer; }
		const char8* str() const { return m_char_count == 0 ? nullptr : m_buffer; }

		void append(char8 chr);
		void append(const char8* str);
		void append(const char8* start, const char8* end);

		// Resets the string with a new src
		// if src == nullptr string will be empty
		void reset(const char8* src = nullptr);

		// Returns the lenth of the string ( or character count )
		rsize len() const { return m_char_count; }

		rsize max_len() const { return kCharCount; }

		bool empty() const { return m_char_count == 0; }
	private:
		void _append(const char8* str);
		void _append_range(const char8* start, const char8* end);

		char8 m_buffer[kCharCount + 1];
		rsize m_char_count;
	};

#if CAMY_ENABLE_LOGGING > 0
	// Overload for dumping to std::ostream
	template <rsize kCharCount>
	std::ostream& operator<<(std::ostream& os, const StaticString<kCharCount>& ss)
	{
		const char8* str = ss.str();
		if (str != nullptr)
			os << ss.str();
		return os;
	}
#endif

	template <rsize kCharCount>
	inline StaticString<kCharCount>::StaticString() :
		m_char_count(0)
	{
		m_buffer[0] = (char8)'\0';
	}

	template <rsize kCharCount>
	inline StaticString<kCharCount>::StaticString(const char8* src)
	{
		m_char_count = 0;
		_append(src);
	}

	template <rsize kCharCount>
	inline StaticString<kCharCount>::StaticString(const char8* start, const char8* end)
	{
		m_char_count = 0;
		_append_range(start, end);
	}

	template <rsize kCharCount>
	inline StaticString<kCharCount>::StaticString(const StaticString& other)
	{
		m_char_count = 0;
		_append(other.str());
	}

	template <rsize kCharCount>
	inline StaticString<kCharCount>& StaticString<kCharCount>::operator=(const StaticString& other)
	{
		m_char_count = 0;
		_append(other.str());
		return *this;
	}

	template <rsize kCharCount>
	inline bool StaticString<kCharCount>::operator==(const char8* str)
	{
		return equals(str);
	}

	template <rsize kCharCount>
	inline bool StaticString<kCharCount>::equals(const StaticString<kCharCount>& other)
	{
		return ::camy::strncmp(m_buffer, other.m_buffer, kCharCount);
	}

	template <rsize kCharCount>
	inline bool StaticString<kCharCount>::equals(const char8* other)
	{
		return strcmp(m_buffer, other) == 0;
	}

	template <rsize kCharCount>
	inline void StaticString<kCharCount>::append(char8 chr)
	{
		if (m_char_count < kCharCount - 1)
		{
			m_buffer[m_char_count] = chr;
			m_buffer[++m_char_count] = (char8)'\0';
		}
	}

	template <rsize kCharCount>
	inline void StaticString<kCharCount>::append(const char8* str)
	{
		_append(str);
	}

	template <rsize kCharCount>
	inline void StaticString<kCharCount>::append(const char8* start, const char8* end)
	{
		_append_range(start, end);
	}

	template <rsize kCharCount>
	inline void StaticString<kCharCount>::reset(const char8* src)
	{
		m_char_count = 0;
		_append(src);
	}

	template <rsize kCharCount>
	inline void StaticString<kCharCount>::_append(const char8* str)
	{
		if (str == nullptr)
			return;
		
		rsize slen = API::min(API::strlen(str), kCharCount);
		memcpy(m_buffer + m_char_count, str, slen);
		m_char_count += slen;
		m_buffer[m_char_count] = 0;
	}
	
	template <rsize kCharCount>
	void StaticString<kCharCount>::_append_range(const char8* start, const char8* end)
	{
		if (start == nullptr || end == nullptr || start == end)
			return;

		rsize slen = API::min((rsize)(end - start), kCharCount);
		memcpy(m_buffer + m_char_count, start, slen);
		m_char_count += slen;
		m_buffer[m_char_count] = 0;
	}
}