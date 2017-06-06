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
    template <typename T>
    class CAMY_API Stack final
    {
    public:
        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_CAPACITY = 64;

        using TStack = Stack<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        Stack(rsize capacity = DEFAULT_CAPACITY, rsize alignment = DEFAULT_ALIGNMENT);
        ~Stack();

        Stack(const TStack& other);
        Stack(TStack&& other);

        TStack& operator=(const TStack& other);
        TStack& operator=(TStack&& other);

        void push(TConstRef el);
        void push(T&& el);
        TRef pop();
        void clear();
        rsize count() const;
        rsize capacity() const;
        bool empty() const;

    private:
        TPtr _allocate_align_explicit(rsize n, rsize alignment);
        TPtr _allocate_align_same(rsize n, void* src_alignment);
        void _deallocate(TPtr ptr);
		void _make_space(rsize n);

        TPtr m_base;
        TPtr m_cur;
        TPtr m_top;
    };

    template <typename T>
    CAMY_INLINE Stack<T>::Stack(rsize capacity, rsize alignment)
        : m_base(nullptr)
        , m_cur(nullptr)
        , m_top(nullptr)
    {
        m_base = _allocate_align_explicit(capacity, alignment);
        m_cur = m_base;
        m_top = m_base + capacity;
    }

    template <typename T>
    CAMY_INLINE Stack<T>::~Stack()
    {
        _deallocate(m_base);
    }

    template <typename T>
    CAMY_INLINE Stack<T>::Stack(const TStack& other)
    {
        m_base = _allocate_align_same(other.capacity(), other.m_base);
        m_top = m_base + other.capacity();

        m_cur = m_base;
        for (rsize i = 0; i < other.count(); ++i)
            new (m_cur++) T(other.m_base + [i]);
    }

    template <typename T>
    CAMY_INLINE Stack<T>::Stack(TStack&& other) :
		m_base(nullptr),
		m_cur(nullptr),
		m_top(nullptr)
    {
		API::swap(m_base, other.m_base);
		API::swap(m_cur, other.m_cur);
		API::swap(m_top, other.m_top);
    }

    template <typename T>
    CAMY_INLINE typename Stack<T>::TStack& Stack<T>::operator=(const TStack& other)
    {
		_deallocate(m_base);
		m_base = _allocate_align_explicit(other.capacity, other.m_base);
        m_top = m_base + other.capacity();
        m_cur = m_base;
        for (rsize i = 0; i < other.count(); ++i)
            new (m_cur++) T(other.m_base + [i]);
        return *this;
    }

    template <typename T>
    CAMY_INLINE typename Stack<T>::TStack& Stack<T>::operator=(TStack&& other)
    {
		_deallocate(m_base);
		m_base = m_cur = m_top = nullptr;
		API::swap(m_base, other.m_base);
		API::swap(m_cur, other.m_cur);
		API::swap(m_top, other.m_top);
        return *this;
    }

    template <typename T>
    CAMY_INLINE void Stack<T>::push(TConstRef el)
    {
		_make_space(count() + 1);
        new (m_cur++) T(el);
    }

    template <typename T>
    CAMY_INLINE void Stack<T>::push(T&& el)
    {
		_make_space(count() + 1);
        new (m_cur++) T(el);
    }

    template <typename T>
    CAMY_INLINE typename Stack<T>::TRef Stack<T>::pop()
    {
        CAMY_ASSERT(m_cur != m_base);
        return *(--m_cur);
    }

    template <typename T>
    CAMY_INLINE void Stack<T>::clear()
    {
        TPtr cur = m_cur;
        while (cur != m_base)
            (cur++)->~T();
        m_cur = m_base;
    }

    template <typename T>
    CAMY_INLINE rsize Stack<T>::count() const
    {
        return (rsize)(m_cur - m_base);
    }

    template <typename T>
	CAMY_INLINE rsize Stack<T>::capacity() const
    {
        return (rsize)(m_top - m_base);
    }

    template <typename T>
    CAMY_INLINE bool Stack<T>::empty() const
    {
        return count() == 0;
    }

	template <typename T>
	CAMY_INLINE typename Stack<T>::TPtr Stack<T>::_allocate_align_explicit(rsize n, rsize alignment)
	{
		return (TPtr)API::allocate(CAMY_ALLOC(n * ELEMENT_SIZE, alignment));
	}
	
	template <typename T>
	CAMY_INLINE typename Stack<T>::TPtr Stack<T>::_allocate_align_same(rsize n, void* src_alignment)
	{
		return (TPtr)API::allocate(CAMY_ALLOC_SRC(n * ELEMENT_SIZE, src_alignment));
	}
	
	template <typename T>
	CAMY_INLINE void Stack<T>::_deallocate(TPtr ptr)
	{
		API::deallocate(ptr);
	}

	template <typename T>
	CAMY_INLINE void Stack<T>::_make_space(rsize n)
	{
		if (count() >= capacity())
		{
			TPtr old_buffer = m_base;
			rsize old_capacity = capacity();
			rsize new_capacity = old_capacity * 2;
			m_base = _allocate_align_same(new_capacity, old_buffer);
			m_top = m_base + new_capacity;
			m_cur = m_base;
			for (rsize i = 0; i < old_capacity; ++i)
				new (m_cur++) T(std::move(old_buffer[i]));
			_deallocate(old_buffer);
		}
	}
}