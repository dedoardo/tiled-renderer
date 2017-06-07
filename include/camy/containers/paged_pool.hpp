/* paged_pool.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/containers/pages.hpp>
#include <camy/system.hpp>

namespace camy
{
    //! Paged pool that preserves pointers
    //! - Typed, thus Constructors and Destructors are called
    //! - Not made for iteration
    //! - Pointers are the indices themselves
    template <typename T>
    class CAMY_API PagedPool final
    {
    public:
        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_PP_CAPACITY = 128;

        using TPage = FreelistPage<T>;
        using TPagedPool = PagedPool<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        PagedPool(rsize elements_per_page = DEFAULT_PP_CAPACITY,
                  rsize alignment = DEFAULT_ALIGNMENT);
        ~PagedPool();

        PagedPool(TPagedPool& other) = delete;
        PagedPool(TPagedPool&& other);

        TPagedPool& operator=(TPagedPool& other) = delete;
        TPagedPool& operator=(TPagedPool&& other) = delete;

        template <typename... Ts>
        TPtr allocate(Ts&&... args);

        void deallocate(TPtr& ptr);
        void clear();

    private:
        void _append_page(rsize alignment = DEFAULT_ALIGNMENT);

        TPage* m_first;
        TPage* m_cur;
        rsize m_elements_per_page;
    };

    template <typename T>
    CAMY_INLINE PagedPool<T>::PagedPool(rsize elements_per_page, rsize alignment)
        : m_first(nullptr)
        , m_cur(nullptr)
        , m_elements_per_page(elements_per_page)
    {
        _append_page(alignment);
    }

    template <typename T>
    CAMY_INLINE PagedPool<T>::~PagedPool()
    {
        TPage* cur = m_first;
        while (cur != nullptr)
        {
            cur->clear();
            TPage* old = cur;
            cur = (TPage*)cur->next;
            API::tdeallocate(old);
        }
    }

    template <typename T>
    CAMY_INLINE PagedPool<T>::PagedPool(TPagedPool&& other)
    {
        m_first = other.m_first;
        m_cur = other.m_cur;
        m_elements_per_page = other.m_elements_per_page;
        other.m_first = nullptr;
        other.m_cur = nullptr;
        other.m_elements_per_page = 0;
    }

    template <typename T>
    template <typename... Ts>
    CAMY_INLINE typename PagedPool<T>::TPtr PagedPool<T>::allocate(Ts&&... args)
    {
        CAMY_ASSERT(m_cur != nullptr);
        // Trying to allocate on current page
        // Very likely it is here
        TPtr ret = m_cur->allocate(std::forward<Ts>(args)...);
        if (ret != nullptr)
            return ret;

        // We scan from the first page on to see if there is a free slot
        // this might not be the best idea, but drastically reduces fragmentation
        TPage* cur = m_first;
        while (cur != nullptr)
        {
            ret = cur->allocate(std::forward<Ts>(args)...);

            // Found a free slot, we also update m_cur this way the next allocate()
            // We might be able to have another free slot on this page
            if (ret != nullptr)
            {
                m_cur = cur;
                return ret;
            }

            cur = (TPage*)cur->next;
        }

        // If we are here it means that we are completely full, we thus need a new page
        _append_page();
        return m_cur->allocate(std::forward<Ts>(args)...);
    }

    template <typename T>
    CAMY_INLINE void PagedPool<T>::deallocate(TPtr& ptr)
    {
        TPage* cur = m_first;
        while (cur != nullptr)
        {
            // Checking if pointer is in this page
            if (cur->is_in_range(ptr))
            {
                cur->deallocate(ptr);
                m_cur = cur;
                return;
            }
        }

        CAMY_ASSERT(false); // Should have found pointer
    }

    template <typename T>
    CAMY_INLINE void PagedPool<T>::clear()
    {
        TPage* cur = m_first;
        while (cur != nullptr)
        {
            cur->clear();
            cur = (TPage*)cur->next;
        }
    }

    template <typename T>
    CAMY_INLINE void PagedPool<T>::_append_page(rsize alignment)
    {
        if (m_cur == nullptr)
        {
            CAMY_ASSERT(m_first == nullptr);
            m_first = API::tallocate<TPage>(CAMY_ALLOC1(alignment), m_elements_per_page, alignment);
            m_cur = m_first;
        }
        else
        {
            TPage* old = m_cur;
            m_cur = API::tallocate<TPage>(CAMY_ALLOC1_SRC(old), m_elements_per_page,
                                          API::memory_block_alignment(old));
            old->next = m_cur;
            m_cur->previous = old;
        }
    }
}