/* paged_linear_vector.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/containers/pages.hpp>
#include <camy/core.hpp>

namespace camy
{
	// Paged version of a DynLinearArray. 
	//! - Preserves pointer as NO contiguous storage
	//! - data() and subscript operator are NOT present.
	//! - Not strictly typed, see methods for more info
	//! It's basically a Pool for pointers that is resettable every frame.
    template <typename T>
    class CAMY_API PagedDynLinearArray final
    {
    public:
        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_PP_CAPACITY = 128;

        using TPage = LinearPage<T>;
        using TPagedDynLinearArray = PagedDynLinearArray<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        PagedDynLinearArray(rsize elements_per_page = DEFAULT_PP_CAPACITY,
                            rsize alignment = DEFAULT_ALIGNMENT);
        ~PagedDynLinearArray();

        PagedDynLinearArray(TPagedDynLinearArray&) = delete;
        PagedDynLinearArray(TPagedDynLinearArray&& other);

        TPagedDynLinearArray& operator=(TPagedDynLinearArray&) = delete;
		TPagedDynLinearArray& operator=(TPagedDynLinearArray&& other) = delete;

		TRef last();
		TConstRef last()const;

		// Constructs a new element in place a returns a reference to it
		template <typename ...Ts>
		TRef emplace(Ts&& ...args);

		// Bumps pointer ahead and returns reference to the new element.
		//! DOES NOT CALL CONSTRUCTOR
		//! To call a constructor use emplace()
		TRef next();
		
		// Bumps pointer ahead count times. As with next(), no constructor
		// is called, instead a pointer to the first element is returned
        TPtr next_array(rsize count);
        void reset();

    private:
        void _append_page(rsize alignment = DEFAULT_ALIGNMENT);

        TPage* m_cur;
        rsize m_elements_per_page;
    };

    template <typename T>
    CAMY_INLINE PagedDynLinearArray<T>::PagedDynLinearArray(rsize elements_per_page,
                                                            rsize alignment)
        : m_cur(nullptr)
        , m_elements_per_page(elements_per_page)
    {
        _append_page(alignment);
    }

    template <typename T>
    CAMY_INLINE PagedDynLinearArray<T>::~PagedDynLinearArray()
    {
        // Simplifies the release by making sure we are at index 0
        reset();

        TPage* cur = m_cur;
        while (cur != nullptr)
        {
            cur->reset();
            TPage* old = cur;
            cur = (TPage*)cur->next;
            API::tdeallocate(old);
        }
    }

    template <typename T>
    CAMY_INLINE PagedDynLinearArray<T>::PagedDynLinearArray(TPagedDynLinearArray&& other)
    {
        m_cur = other.m_cur;
        m_elements_per_page = other.m_elements_per_page;
        other.m_cur = nullptr;
        other.m_elements_per_page = 0;
    }

	template <typename T>
	CAMY_INLINE typename PagedDynLinearArray<T>::TRef PagedDynLinearArray<T>::last()
	{
		return *m_cur->last();
	}

	template <typename T>
	CAMY_INLINE typename PagedDynLinearArray<T>::TConstRef PagedDynLinearArray<T>::last()const
	{
		return *m_cur->last();
	}

	template <typename T>
	template <typename ...Ts>
	CAMY_INLINE typename PagedDynLinearArray<T>::TRef PagedDynLinearArray<T>::emplace(Ts&& ...args)
	{
		TPtr ret = m_cur->next_single();

		if (ret == nullptr)
		{
			if (m_cur->next == nullptr) _append_page();
			ret = m_cur->next_single();
		}

		new (ret) T(std::forward<Ts>(args)...);
		return *ret;
	}

    template <typename T>
    CAMY_INLINE typename PagedDynLinearArray<T>::TRef PagedDynLinearArray<T>::next()
    {
        TPtr ret = m_cur->next_single();

        // Next page **has** to have a free slot
        if (ret == nullptr)
        {
            // Append new page if at last
            if (m_cur->next == nullptr) _append_page();
            ret = m_cur->next_single();
        }

        return *ret;
    }

    template <typename T>
    CAMY_INLINE typename PagedDynLinearArray<T>::TPtr PagedDynLinearArray<T>::next_array(rsize count)
    {
        if (count == 0) return nullptr;
        TPtr ret = m_cur->next_array(count);

        // Next page **has** to have a free slot
        if (ret == nullptr)
        {
            // Append new page if at last
            if (m_cur->next == nullptr)
                _append_page();
            else
                m_cur = (TPage*)m_cur->next;
            ret = m_cur->next_array(count);
        }
        CAMY_ASSERT(ret != nullptr);
        return ret;
    }

    template <typename T>
    CAMY_INLINE void PagedDynLinearArray<T>::reset()
    {
        TPage* prev = m_cur; // from cur onwards they should be reset already from previous call
        while (prev != nullptr)
        {
            prev->reset();
            m_cur = prev;
            prev = (TPage*)prev->previous;
        }
    }

    template <typename T>
    CAMY_INLINE void PagedDynLinearArray<T>::_append_page(rsize alignment)
    {
        if (m_cur == nullptr)
        {
            m_cur = API::tallocate<TPage>(CAMY_ALLOC1(alignment), m_elements_per_page, alignment);
        }
        else
        {
            TPage* old = m_cur;
            m_cur = API::tallocate<TPage>(CAMY_ALLOC1_SRC(old), m_elements_per_page, API::memory_block_alignment(old));
            old->next = m_cur;
            m_cur->previous = old;
        }
    }
}