/* linear_array.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core.hpp>

namespace camy
{
	/*
		Stack allocated arena allocator. Stack counterpart of linear_vector.
	*/
	template <typename ElementType, rsize kElementCount, uint16 kAlignment = DEFAULT_ALIGNMENT>
	class CAMY_API alignas(kAlignment)LinearArray final
	{
	public:
		LinearArray();
		~LinearArray() = default;

		LinearArray(LinearArray&& other) = default;
		LinearArray(LinearArray& other) = default;

		LinearArray& operator=(LinearArray&& other) = default;
		LinearArray& operator=(LinearArray& other) = default;

		ElementType& operator[](rsize idx);
		const ElementType& operator[](rsize idx)const;

		ElementType* next();
		void         reset();
		ElementType* data(); // ptr to 0th item

		rsize count()const;
		rsize capacity()const;
	private:
		rsize m_num_elements;
		ElementType m_buffer[kElementCount];
	};

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline LinearArray<ElementType, kElementCount, kAlignment>::LinearArray() :
		m_num_elements(0)
	{

	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline ElementType& LinearArray<ElementType, kElementCount, kAlignment>::operator[](rsize idx)
	{
		assert(idx < m_num_elements);
		return m_buffer[idx];
	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline const ElementType& LinearArray<ElementType, kElementCount, kAlignment>::operator[](rsize idx)const
	{
		assert(idx < m_num_elements);
		return m_buffer[idx];
	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline ElementType* LinearArray<ElementType, kElementCount, kAlignment>::next()
	{
		CAMY_ASSERT(m_num_elements < kElementCount);

		ElementType* ret = &m_buffer[m_num_elements++];
		new (ret) ElementType();
		return ret;
	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline void LinearArray<ElementType, kElementCount, kAlignment>::reset()
	{
		m_num_elements = 0;
	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline ElementType* LinearArray<ElementType, kElementCount, kAlignment>::data()
	{
		return m_buffer;
	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline rsize LinearArray<ElementType, kElementCount, kAlignment>::count() const
	{
		return m_num_elements;
	}

	template<typename ElementType, rsize kElementCount, uint16 kAlignment>
	inline rsize LinearArray<ElementType, kElementCount, kAlignment>::capacity() const
	{
		return kElementCount;
	}
}