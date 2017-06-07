/* pool.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/containers/stack.hpp>
#include <camy/system.hpp>

namespace camy
{
    // Typed pool that uses indices instead of pointers as storage is contiguous
    // and pointers are likely to be invalidate. get() is as quick as indexing an array though.
    template <typename T>
    class CAMY_API Pool final
    {
    public:
        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_CAPACITY = 128;

        using TPool = Pool<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        Pool(rsize capacity = DEFAULT_CAPACITY, rsize alignment = DEFAULT_ALIGNMENT);
        ~Pool();

        Pool(const TPool&) = delete;
        Pool(TPool&& other);

        TPool& operator=(const TPool&) = delete;
        TPool& operator=(TPool&& other) = delete;

        rsize reserve();

        template <typename... Ts>
        rsize allocate(Ts&&... args);

        TRef get(rsize idx);
        TConstRef& get(rsize idx) const;

        void deallocate(rsize idx);
        void clear();
        rsize capacity() const;

    private:
        TPtr _allocate_align_explicit(rsize n, rsize alignment);
        TPtr _allocate_align_same(rsize n, void* src_alignment);
        void _deallocate(TPtr ptr);
        void _grow();

        TPtr m_base;
        TPtr m_top;
        Stack<rsize> m_freelist;
    };

    template <typename T>
    CAMY_INLINE Pool<T>::Pool(rsize capacity, rsize alignment)
        : m_base(nullptr)
        , m_top(nullptr)
    {
        m_base = _allocate_align_explicit(capacity, alignment);
        m_top = m_base + capacity;
        for (rsize i = 0; i < capacity; ++i)
            m_freelist.push(capacity - i - 1);
    }

    template <typename T>
    CAMY_INLINE Pool<T>::~Pool()
    {
        _deallocate(m_base);
    }

    template <typename T>
    CAMY_INLINE Pool<T>::Pool(TPool&& other)
    {
        m_base = other.m_base;
        m_top = other.m_top;
        m_freelist = std::move(other.m_freelist);
        other.m_base = nullptr;
        other.m_top = nullptr;
    }

    template <typename T>
    CAMY_INLINE rsize Pool<T>::reserve()
    {
        if (m_freelist.empty())
            _grow();

        rsize ret = m_freelist.pop();
        new (m_base + ret) T();
        return ret;
    }

    template <typename T>
    template <typename... Ts>
    CAMY_INLINE rsize Pool<T>::allocate(Ts&&... args)
    {
        if (m_freelist.empty())
            _grow();

        rsize ret = m_freelist.pop();
        new (m_base + ret) T(std::forward<Ts>(args)...);
        return ret;
    }

    template <typename T>
    CAMY_INLINE typename Pool<T>::TRef Pool<T>::get(rsize idx)
    {
        CAMY_ASSERT(idx < capacity());
        return *(m_base + idx);
    }

    template <typename T>
    CAMY_INLINE typename Pool<T>::TConstRef Pool<T>::get(rsize idx) const
    {
        CAMY_ASSERT(idx < capacity());
        return *(m_base + idx);
    }

    template <typename T>
    CAMY_INLINE void Pool<T>::deallocate(rsize idx)
    {
        m_freelist.push(idx);
    }

    template <typename T>
    CAMY_INLINE void Pool<T>::clear()
    {
        m_freelist.clear();
        for (rsize i = 0; i < capacity(); ++i)
            m_freelist.push(capacity() - i - 1);
    }

    template <typename T>
    CAMY_INLINE rsize Pool<T>::capacity() const
    {
        return (rsize)(m_top - m_base);
    }

    template <typename T>
    CAMY_INLINE typename Pool<T>::TPtr Pool<T>::_allocate_align_explicit(rsize n, rsize alignment)
    {
        return (TPtr)API::allocate(CAMY_ALLOC(n * ELEMENT_SIZE, alignment));
    }

    template <typename T>
    CAMY_INLINE typename Pool<T>::TPtr Pool<T>::_allocate_align_same(rsize n, void* src_alignment)
    {
        return (TPtr)API::allocate(CAMY_ALLOC_SRC(n * ELEMENT_SIZE, src_alignment));
    }

    template <typename T>
    CAMY_INLINE void Pool<T>::_deallocate(TPtr ptr)
    {
        API::deallocate(ptr);
    }

    template <typename T>
    CAMY_INLINE void Pool<T>::_grow()
    {
        TPtr old_buffer = m_base;
        rsize old_capacity = capacity();
        rsize new_capacity = old_capacity * 2;
        m_base = _allocate_align_same(new_capacity, old_buffer);
        m_top = m_base + new_capacity;
        for (rsize i = 0; i < old_capacity; ++i)
        {
            new (m_base + i) T(std::move(old_buffer[i]));
            m_freelist.push(new_capacity - i - 1);
        }
        _deallocate(old_buffer);
    }
}