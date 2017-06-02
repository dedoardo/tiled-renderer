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
    /*
            Class: PagedPool
                    Paged pool allocators that does not invalidate pointers, iterating
                    all the elements might be inefficient depending on fragmentation.
                    Especially useful for resource managers that want to return pointer
                    to resources.

                    Note: Does call destructors as it relies on FreelistPage

                    Note: Actual memory is released only on destruction
    */
    template <typename ElementType, uint16 kAlignment = DEFAULT_ALIGNMENT>
    class CAMY_API PagedPool final
    {
      public:
        using PageType = FreelistPage<ElementType>;

      public:
        PagedPool(rsize elements_per_page = 100);
        ~PagedPool();

        PagedPool(PagedPool&& other);
        PagedPool(PagedPool& other) = delete;

        PagedPool& operator=(PagedPool&& other);
        PagedPool& operator=(PagedPool& other) = delete;

        template <typename... CtorArgs>
        ElementType* allocate(CtorArgs&&... ctor_args);

        void deallocate(ElementType*& ptr);
        void clear();

      private:
        void _append_page();

        PageType* m_first;
        PageType* m_cur;
        rsize m_elements_per_page;
    };

    template <typename ElementType, uint16 kAlignment>
    inline PagedPool<ElementType, kAlignment>::PagedPool(rsize elements_per_page)
        : m_first(nullptr), m_cur(nullptr), m_elements_per_page(elements_per_page)
    {
        _append_page();
    }

    template <typename ElementType, uint16 kAlignment>
    inline PagedPool<ElementType, kAlignment>::~PagedPool()
    {
        PageType* cur = m_first;
        while (cur != nullptr)
        {
            cur->clear();
            cur = (FreelistPage<ElementType>*)cur->next;
            tdeallocate(cur);
        }
    }

    template <typename ElementType, uint16 kAlignment>
    inline PagedPool<ElementType, kAlignment>::PagedPool(PagedPool&& other)
    {
        m_first = other.m_first;
        m_cur = other.m_cur;
        m_elements_per_page = other.m_elements_per_page;
        other.m_first = nullptr;
        other.m_cur = nullptr;
        other.m_elements_per_page = 0;
    }

    template <typename ElementType, uint16 kAlignment>
    inline PagedPool<ElementType, kAlignment>& PagedPool<ElementType, kAlignment>::
    operator=(PagedPool&& other)
    {
        m_first = other.m_first;
        m_cur = other.m_cur;
        m_elements_per_page = other.m_elements_per_page;
        other.m_first = nullptr;
        other.m_cur = nullptr;
        other.m_elements_per_page = 0;
        return *this;
    }

    template <typename ElementType, uint16 kAlignment>
    template <typename... CtorArgs>
    inline ElementType* PagedPool<ElementType, kAlignment>::allocate(CtorArgs&&... ctor_args)
    {
        CAMY_ASSERT(m_cur != nullptr);
        // Trying to allocate on current page
        // Very likely it is here
        ElementType* ret = m_cur->allocate(std::forward<CtorArgs>(ctor_args)...);
        if (ret != nullptr) return ret;

        // We scan from the first page on to see if there is a free slot
        // this might not be the best idea, but drastically reduces fragmentation
        PageType* cur = m_first;
        while (cur != nullptr)
        {
            ret = cur->allocate(std::forward<CtorArgs>(ctor_args)...);

            // Found a free slot, we also update m_cur this way the next allocate()
            // We might be able to have another free slot on this page
            if (ret != nullptr)
            {
                m_cur = cur;
                return ret;
            }

            cur = (FreelistPage<ElementType>*)cur->next;
        }

        // If we are here it means that we are completely full, we thus need a new page
        _append_page();
        return m_cur->allocate(std::forward<CtorArgs>(ctor_args)...);
    }

    template <typename ElementType, uint16 kAlignment>
    inline void PagedPool<ElementType, kAlignment>::deallocate(ElementType*& ptr)
    {
        PageType* cur = m_first;
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

    template <typename ElementType, uint16 kAlignment>
    inline void PagedPool<ElementType, kAlignment>::clear()
    {
        PageType* cur = m_first;
        while (cur != nullptr)
        {
            cur->clear();
            cur = (PageType*)cur->next;
        }
    }

    template <typename ElementType, uint16 kAlignment>
    inline void PagedPool<ElementType, kAlignment>::_append_page()
    {
        if (m_cur == nullptr)
        {
            CAMY_ASSERT(m_first == nullptr);
            m_first = tallocate<PageType>(camy_loc, kAlignment, m_elements_per_page);
            m_cur = m_first;
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