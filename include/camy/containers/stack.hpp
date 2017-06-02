/* stack.hpp
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
    template <typename ElementType, uint16 kAlignment = DEFAULT_ALIGNMENT>
    class CAMY_API Stack final
    {
      public:
        static const rsize kElementSize = sizeof(ElementType);

      public:
        Stack(rsize initial_capacity = 2);
        ~Stack();

        Stack(Stack&& other);
        Stack(Stack& other);

        Stack& operator=(Stack&& other);
        Stack& operator=(const Stack& other);

        void push(const ElementType& el);
        void push(ElementType&& el);
        ElementType& pop();
        void clear();
        rsize count() const;
        rsize capacity() const;
        bool empty() const;

      private:
        void _realloc();

        ElementType* m_base;
        ElementType* m_cur;
        ElementType* m_top;
    };

    template <typename ElementType, uint16 kAlignment>
    inline Stack<ElementType, kAlignment>::Stack(rsize initial_capacity)
        : m_base(nullptr), m_cur(nullptr)
    {
        m_base =
            (ElementType*)API::allocate(CAMY_ALLOC(kElementSize * initial_capacity, kAlignment));
        m_cur = m_base;
        m_top = m_base + initial_capacity;
    }

    template <typename ElementType, uint16 kAlignment>
    inline Stack<ElementType, kAlignment>::~Stack()
    {
        API::deallocate(m_base);
    }

    template <typename ElementType, uint16 kAlignment>
    inline Stack<ElementType, kAlignment>::Stack(Stack&& other)
    {
        m_base = other.m_base;
        m_cur = other.m_cur;
        m_top = other.m_top;
        other.m_base = other.m_cur = other.m_top = nullptr;
    }

    template <typename ElementType, uint16 kAlignment>
    inline Stack<ElementType, kAlignment>::Stack(Stack& other)
    {
        m_base =
            (ElementType*)API::allocate(CAMY_ALLOC(kElementSize * other.capacity(), kAlignment));
        m_top = m_base + other.capacity();

        m_cur = m_base;
        for (rsize i = 0; i < other.count(); ++i)
            new (m_cur++) ElementType(other.m_base + [i]);
    }

    template <typename ElementType, uint16 kAlignment>
    inline Stack<ElementType, kAlignment>& Stack<ElementType, kAlignment>::operator=(Stack&& other)
    {
        m_base = other.m_base;
        m_cur = other.m_cur;
        m_top = other.m_top;
        other.m_base = other.m_cur = other.m_top = nullptr;
        return *this;
    }

    template <typename ElementType, uint16 kAlignment>
    inline Stack<ElementType, kAlignment>& Stack<ElementType, kAlignment>::
    operator=(const Stack& other)
    {
        m_base = (ElementType*)allocate(camy_loc, kElementSize * other.capacity(), kAlignment);
        m_top = m_base + other.capacity();
        m_cur = m_base;
        for (rsize i = 0; i < other.count(); ++i)
            new (m_cur++) ElementType(other.m_base + [i]);
        return *this;
    }

    template <typename ElementType, uint16 kAlignment>
    inline void Stack<ElementType, kAlignment>::push(const ElementType& el)
    {
        if (m_cur == m_top) _realloc();

        new (m_cur++) ElementType(el);
    }

    template <typename ElementType, uint16 kAlignment>
    inline void Stack<ElementType, kAlignment>::push(ElementType&& el)
    {
        if (m_cur == m_top) _realloc();

        new (m_cur++) ElementType(el);
    }

    template <typename ElementType, uint16 kAlignment>
    inline ElementType& Stack<ElementType, kAlignment>::pop()
    {
        CAMY_ASSERT(m_cur != m_base);
        return *(--m_cur);
    }

    template <typename ElementType, uint16 kAlignment>
    inline void Stack<ElementType, kAlignment>::clear()
    {
        ElementType* cur = m_cur;
        while (cur != m_base)
            (cur++)->~ElementType();
        m_cur = m_base;
    }

    template <typename ElementType, uint16 kAlignment>
    inline rsize Stack<ElementType, kAlignment>::count() const
    {
        return (rsize)(m_cur - m_base);
    }

    template <typename ElementType, uint16 kAlignment>
    inline rsize Stack<ElementType, kAlignment>::capacity() const
    {
        return (rsize)(m_top - m_base);
    }

    template <typename ElementType, uint16 kAlignment>
    inline bool Stack<ElementType, kAlignment>::empty() const
    {
        return count() == 0;
    }

    template <typename ElementType, uint16 kAlignment>
    inline void Stack<ElementType, kAlignment>::_realloc()
    {
        ElementType* old_buffer = m_base;
        rsize old_capacity = capacity();
        rsize new_capacity = old_capacity * 2;
        m_base = (ElementType*)API::allocate(CAMY_ALLOC(kElementSize * new_capacity, kAlignment));
        m_top = m_base + new_capacity;
        m_cur = m_base;
        for (rsize i = 0; i < old_capacity; ++i)
            new (m_cur++) ElementType(std::move(old_buffer[i]));

        API::deallocate(old_buffer);
    }
}