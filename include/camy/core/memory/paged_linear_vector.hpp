/* paged_linear_vector.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/memory/alloc.hpp>
#include <camy/core/memory/pages.hpp>

namespace camy
{
	/*
		Paged version of a linear vector. Preserves pointers.
		API is **much** simpler as memory is not contiguous and
		methods such as data() would be misleading.
	*/
	template <typename ElementType, uint16 kAlignment = kDefaultAlignment>
	class camy_api PagedLinearVector final
	{
	public:
		using PageType = LinearPage<ElementType>;

	public:
		PagedLinearVector(rsize elements_per_page = 100);
		~PagedLinearVector();

		PagedLinearVector(PagedLinearVector&& other);
		PagedLinearVector(PagedLinearVector& other) = delete; // TODO

		PagedLinearVector& operator=(PagedLinearVector&& other);
		PagedLinearVector& operator=(PagedLinearVector& other) = delete; // TODO

		ElementType& next();
		ElementType* next_array(rsize count);
		void reset();
	private:
		void _append_page();

		PageType* m_cur;
		rsize m_elements_per_page;
	};

	template<typename ElementType, uint16 kAlignment>
	inline PagedLinearVector<ElementType, kAlignment>::PagedLinearVector(rsize elements_per_page) : 
		m_cur(nullptr),
		m_elements_per_page(elements_per_page)
	{
		_append_page();
	}

	template<typename ElementType, uint16 kAlignment>
	inline PagedLinearVector<ElementType, kAlignment>::~PagedLinearVector()
	{
		// Simplifies the release by making sure we are at index 0
		reset();

		PageType* cur = m_cur;
		while (cur != nullptr)
		{
			cur->reset();
			PageType* old = cur;
			cur = (PageType*)cur->next;
			deallocate(old);
		}
	}

	template<typename ElementType, uint16 kAlignment>
	inline PagedLinearVector<ElementType, kAlignment>::PagedLinearVector(PagedLinearVector&& other)
	{
		m_cur = other.m_cur;
		m_elements_per_page = other.m_elements_per_page;
		other.m_cur = nullptr;
		other.m_elements_per_page = 0;
	}

	template<typename ElementType, uint16 kAlignment>
	inline PagedLinearVector<ElementType, kAlignment> & PagedLinearVector<ElementType, kAlignment>::operator=(PagedLinearVector&& other)
	{
		m_cur = other.m_cur;
		m_elements_per_page = other.m_elements_per_page;
		other.m_cur = nullptr;
		other.m_elements_per_page = 0;

		return *this;
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType& PagedLinearVector<ElementType, kAlignment>::next()
	{
		ElementType* ret = m_cur->_next();

		// Next page **has** to have a free slot
		if (ret == nullptr)
		{
			// Append new page if at last
			if (m_cur->next == nullptr)
				_append_page();
			ret = m_cur->_next();
		}

		return *ret;
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType * PagedLinearVector<ElementType, kAlignment>::next_array(rsize count)
	{
		if (count == 0) return nullptr;
		ElementType* ret = m_cur->next_array(count);

		// Next page **has** to have a free slot
		if (ret == nullptr)
		{
			// Append new page if at last
			if (m_cur->next == nullptr)
				_append_page();
			else
				m_cur = (PageType*)m_cur->next;
			ret = m_cur->next_array(count);
		}
		camy_assert(ret != nullptr);
		return ret;
	}

	template<typename ElementType, uint16 kAlignment>
	inline void PagedLinearVector<ElementType, kAlignment>::reset()
	{
		PageType* prev = m_cur; // from cur onwards they should be reset already from previous call
		while (prev != nullptr)
		{
			prev->reset();
			m_cur = prev;
			prev = (PageType*)prev->previous;
		}
	}

	template<typename ElementType, uint16 kAlignment>
	inline void PagedLinearVector<ElementType, kAlignment>::_append_page()
	{
		if (m_cur == nullptr)
		{
			m_cur = tallocate<PageType>(camy_loc, kAlignment, m_elements_per_page);
		}
		else
		{
			PageType* old = m_cur;
			m_cur = tallocate<PageType>(camy_loc, kAlignment, m_elements_per_page);
			old->next = m_cur;
			m_cur->previous = old;
		}
	}
}