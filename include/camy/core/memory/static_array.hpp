/* static_array.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>

/*
	Simple wrapper for static arrays allocated on the stack. Interface is minimal
	and here basically to avoid having to manually deal with C-arrays that are
	extremely error prone. There is not overhead for StaticArray.
*/
namespace camy
{
	template <typename ElementType, uint32 kElementCount>
	class camy_api StaticArray final
	{
	public:
		StaticArray() = default;

        StaticArray(StaticArray&& other) = default;
        StaticArray(StaticArray& other) = default;

        StaticArray& operator=(StaticArray&& other) = default;
        StaticArray& operator=(StaticArray& other) = default;

		// TODO: Might write emplacement new version ?
		// Not really needed tho since most contained elements
		// are quite simple
		StaticArray(ElementType val)
		{
			for (uint32 i = 0; i < kElementCount; ++i)
				m_buffer[i] = val;
		}

		uint32 len()const { return kElementCount; }
			
		ElementType& operator[](uint32 idx)
		{
			camy_assert(idx < kElementCount);
			return m_buffer[idx];
		}

		const ElementType& operator[](uint32 idx)const
		{
			camy_assert(idx < kElementCount);
			return m_buffer[idx];
		}
	private:
		ElementType m_buffer[kElementCount];
	};
}