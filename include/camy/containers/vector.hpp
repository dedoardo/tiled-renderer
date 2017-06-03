/* vector.hpp
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
    class CAMY_API Vector final
    {
    public:
        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_CAPACITY = 128;

        using TVector = Vector<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        explicit Vector(rsize capacity = DEFAULT_CAPACITY, rsize alignment = DEFAULT_ALIGNMENT);
        ~Vector();

        Vector(const TVector& other);
        Vector(TVector&& other);

        TVector& operator=(const TVector& other);
        TVector& operator=(TVector&& other);

        TRef operator[](rsize idx);
        TConstRef operator[](rsize idx) const;

        TRef first();
        TConstRef first() const;

        TRef last();
        TConstRef last() const;

        TPtr data();
        TConstPtr data() const;

        void append(TConstRef val);
        void append(T&& val);

        template <typename... Ts>
        void emplace_last(Ts&&... args);

        void resize(rsize);

        void pop_last();
        void remove(rsize idx);
        void clear();

        rsize count() const;
        rsize capacity() const;
        bool empty() const;

    private:
        TPtr _allocate_align_explicit(rsize n, rsize alignment);
        TPtr _allocate_align_same(rsize n, TPtr src_alignment);
        void _destruct(TPtr beg, rsize n);
        void _deallocate(TPtr ptr);
        void _copy_all(TPtr dest_beg, TConstPtr src_beg, rsize n);
        void _move_all(TPtr dest_beg, TPtr src_beg, rsize n);
        void _make_space(rsize n);

        TPtr m_beg;
        TPtr m_cur;
        TPtr m_end;
    };

    template <typename T>
    CAMY_INLINE Vector<T>::Vector(rsize capacity, rsize alignment)
    {
        m_beg = _allocate_align_explicit(capacity, alignment);
        m_cur = m_beg;
        m_end = m_beg + capacity;
    }

    template <typename T>
    CAMY_INLINE Vector<T>::~Vector()
    {
        _destruct(m_beg, count());
        _deallocate(m_beg);
    }

    template <typename T>
    CAMY_INLINE Vector<T>::Vector(const TVector& other)
    {
        m_beg = _allocate_align_same(other.capacity(), other.m_beg);
        _copy_all(m_beg, other.m_beg, other.count());
        m_cur = m_beg + other.count();
        m_end = m_beg + other.capacity();
    }

    template <typename T>
    CAMY_INLINE Vector<T>::Vector(TVector&& other) : Vector()
    {
        API::swap(m_beg, other.m_beg);
        API::swap(m_cur, other.m_cur);
        API::swap(m_end, other.m_end);
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TVector& Vector<T>::operator=(const TVector& other)
    {
        m_beg = _allocate_align_same(other.m_capacity, other.m_beg);
        _copy_all(m_beg, other.m_beg, other.count());
        m_cur = m_beg + other.count();
        m_end = m_beg + other.m_capacity;
        return *this;
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TVector& Vector<T>::operator=(TVector&& other)
    {
        API::swap(m_beg, other.m_beg);
        API::swap(m_cur, other.m_cur);
        API::swap(m_end, other.m_end);
        return *this;
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TRef Vector<T>::operator[](rsize idx)
    {
        CAMY_ASSERT(idx < count());
        return *(m_beg + idx);
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TConstRef Vector<T>::operator[](rsize idx) const
    {
        CAMY_ASSERT(idx < count());
        return *(m_beg + idx);
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TRef Vector<T>::first()
    {
        return *m_beg;
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TConstRef Vector<T>::first() const
    {
        return *m_beg;
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TRef Vector<T>::last()
    {
        return *(m_cur - 1);
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TConstRef Vector<T>::last() const
    {
        return *(m_cur - 1);
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TPtr Vector<T>::data()
    {
        return m_beg;
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TConstPtr Vector<T>::data() const
    {
        return m_beg;
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::append(TConstRef value)
    {
        _make_space(count() + 1);
        new (m_cur++) T(value);
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::append(T&& value)
    {
        _make_space(count() + 1);
        new (m_cur++) T(value);
    }

    template <typename T>
    template <typename... Ts>
    CAMY_INLINE void Vector<T>::emplace_last(Ts&&... args)
    {
        _make_space(count() + 1);
        new (m_cur++) T(std::forward<Ts>(args)...);
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::resize(rsize new_count)
    {
        rsize cur_count = count();

        // Same
        if (cur_count == new_count) return;

        // We have space, destructing last (cur_count - new_count) elements
        if (new_count < cur_count)
        {
            for (rsize i = new_count; i < cur_count; ++i)
                (--m_cur)->~T();
        }
        // Otherwise making space and constructing (new_count - cur_count) elements
        else
        {
            _make_space(new_count);
            for (rsize i = cur_count; i < new_count; ++i)
                new (m_cur++) T();
        }
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::pop_last()
    {
        CAMY_ASSERT(m_cur > m_beg);
        --m_cur;
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::remove(rsize idx)
    {
        CAMY_ASSERT(idx < count());

        TPtr to_remove = m_beg + idx;

        // Destructing old element
        to_remove->~T();

        // Moving last into to_remove position
        new (to_remove) T(std::move(*(--m_cur)));
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::clear()
    {
        _destruct(m_beg, count());
        m_cur = m_beg;
    }

    template <typename T>
    CAMY_INLINE rsize Vector<T>::count() const
    {
        return (rsize)(m_cur - m_beg);
    }

    template <typename T>
    CAMY_INLINE rsize Vector<T>::capacity() const
    {
        return (rsize)(m_end - m_beg);
    }

    template <typename T>
    CAMY_INLINE bool Vector<T>::empty() const
    {
        return m_beg == m_cur;
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TPtr Vector<T>::_allocate_align_explicit(rsize n,
                                                                             rsize alignment)
    {
        return (TPtr)API::allocate(CAMY_ALLOC(n * sizeof(T), alignment));
    }

    template <typename T>
    CAMY_INLINE typename Vector<T>::TPtr Vector<T>::_allocate_align_same(rsize n,
                                                                         TPtr src_alignment)
    {
        return (TPtr)API::allocate(CAMY_ALLOC_SRC(n * sizeof(T), src_alignment));
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::_destruct(TPtr beg, rsize n)
    {
        for (rsize i = 0; i < n; ++i)
            (beg + i)->~T();
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::_deallocate(TPtr ptr)
    {
        API::deallocate(ptr);
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::_copy_all(TPtr dest_beg, TConstPtr src_beg, rsize n)
    {
        for (rsize i = 0; i < n; ++i)
            new (dest_beg + i) T(*(src_beg + i));
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::_move_all(TPtr dest_beg, TPtr src_beg, rsize n)
    {
        for (rsize i = 0; i < n; ++i)
            new (dest_beg + i) T(std::move(*(src_beg + i)));
    }

    template <typename T>
    CAMY_INLINE void Vector<T>::_make_space(rsize n)
    {
        if (n > capacity())
        {
            TPtr old_beg = m_beg;
            rsize old_capacity = capacity();
            rsize new_capacity = API::min(DEFAULT_CAPACITY, old_capacity * 2);
            m_beg = _allocate_align_same(new_capacity, old_beg);

            _move_all(m_beg, old_beg, old_capacity);
            _deallocate(old_beg);
            m_cur = m_beg + old_capacity;
            m_end = m_beg + new_capacity;
        }
    }
}