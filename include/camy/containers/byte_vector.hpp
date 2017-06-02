/* byte_vector.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/system.hpp>

namespace camy
{
	class CAMY_API ByteVector final
	{
	public:
		ByteVector();
		~ByteVector();

		ByteVector(ByteVector&& other);
		ByteVector(const ByteVector& other);

		ByteVector& operator=(ByteVector&& other);
		ByteVector& operator=(const ByteVector& other);

		byte* data();
		template <typename T>
		void append(const T& val);

		void clear();

		rsize count();

	private:
		byte* m_buffer;
		rsize m_size;
		rsize m_capacity;
	};

	template<typename T>
	inline void ByteVector::append(const T & val)
	{
		rsize bytes = sizeof(T);
		if (m_size + bytes > m_capacity)
		{
			byte* old_buffer = m_buffer;
			rsize new_capacity = API::max(m_capacity * 2, bytes);
			m_buffer = (byte*)API::allocate(CAMY_UALLOC(new_capacity));
			memcpy(m_buffer, old_buffer, m_capacity);
			API::deallocate(old_buffer);
			m_capacity = new_capacity;
		}

		memcpy(m_buffer + m_size, &val, sizeof(T));
		m_size += bytes;
	}
}