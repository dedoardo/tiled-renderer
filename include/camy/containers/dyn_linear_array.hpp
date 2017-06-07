/* linear_array.hpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#pragma once

// camy
#include <camy/containers/dyn_linear_buffer.hpp>
#include <camy/system.hpp>

namespace camy
{
    // Typed wrapper around DynLinearBuffer, calls ctor/dtor
    template <typename T>
    class DynLinearArray final : private DynLinearBuffer
    {
    public:
        static const rsize ELEMENT_SIZE = sizeof(T);
        static const rsize DEFAULT_CAPACITY = 128;

        using TDynLinearArray = DynLinearArray<T>;
        using TValue = T;
        using TPtr = T*;
        using TConstPtr = const T*;
        using TRef = T&;
        using TConstRef = const T&;

    public:
        explicit DynLinearArray(rsize capacity = DEFAULT_CAPACITY,
                                rsize alignment = DEFAULT_ALIGNMENT);
        ~DynLinearArray() = default;

        DynLinearArray(const TDynLinearArray& other) = default;
        DynLinearArray(TDynLinearArray&& other) = default;

        TDynLinearArray& operator=(const TDynLinearArray& other) = default;
        TDynLinearArray& operator=(TDynLinearArray&& other) = default;

        TRef operator[](rsize idx);
        TConstRef operator[](rsize idx) const;

        TRef first();
        TConstRef first() const;

        TRef last();
        TConstRef last() const;

        TRef next();

        TPtr data();
        TConstPtr data() const;

        void append(TConstRef val);
        void append(T&& val);

        template <typename... Ts>
        void emplace_last(Ts&&... args);

        void clear();
        rsize count() const;
        rsize capacity() const;
        bool empty() const;
    };

    template <typename T>
    CAMY_INLINE DynLinearArray<T>::DynLinearArray(rsize capacity, rsize alignment)
        : DynLinearBuffer(capacity * ELEMENT_SIZE, alignment)
    {
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TRef DynLinearArray<T>::operator[](rsize idx)
    {
        CAMY_ASSERT(idx < count());
        TPtr tbeg = (TPtr)data();
        return *(tbeg + idx);
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TConstRef DynLinearArray<T>::operator[](rsize idx) const
    {
        CAMY_ASSERT(idx < count());
        TPtr tbeg = (TPtr)m_beg;
        return *(tbeg + idx);
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TRef DynLinearArray<T>::first()
    {
        return *((TPtr)data());
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TConstRef DynLinearArray<T>::first() const
    {
        return *((TPtr)data());
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TRef DynLinearArray<T>::last()
    {
        return *(((TPtr)cur()) - 1);
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TConstRef DynLinearArray<T>::last() const
    {
        return *(((TPtr)cur()) - 1);
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TRef DynLinearArray<T>::next()
    {
        TPtr addr = (TPtr)incr(sizeof(T));
        new (addr) T(); // TODO: Should I call defeault ctor ?
        return *addr;
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TPtr DynLinearArray<T>::data()
    {
        return (TPtr)DynLinearBuffer::data();
    }

    template <typename T>
    CAMY_INLINE typename DynLinearArray<T>::TConstPtr DynLinearArray<T>::data() const
    {
        return (TPtr)DynLinearBuffer::data();
    }

    template <typename T>
    CAMY_INLINE void DynLinearArray<T>::append(TConstRef val)
    {
        TPtr addr = (TPtr)incr(sizeof(T));
        new (addr) T(val);
    }

    template <typename T>
    CAMY_INLINE void DynLinearArray<T>::append(T&& val)
    {
        TPtr addr = (TPtr)incr(sizeof(T));
        new (addr) T(val);
    }

    template <typename T>
    template <typename... Ts>
    CAMY_INLINE void DynLinearArray<T>::emplace_last(Ts&&... args)
    {
        TPtr addr = (TPtr)incr(sizeof(T));
        new (addr) T(std::forward<Ts>(args)...);
    }

    template <typename T>
    CAMY_INLINE void DynLinearArray<T>::clear()
    {
        DynLinearBuffer::clear();
    }

    template <typename T>
    CAMY_INLINE rsize DynLinearArray<T>::count() const
    {
        return DynLinearBuffer::count() / ELEMENT_SIZE;
    }

    template <typename T>
    CAMY_INLINE rsize DynLinearArray<T>::capacity() const
    {
        return DynLinearBuffer::capacity() / ELEMENT_SIZE;
    }

    template <typename T>
    CAMY_INLINE bool DynLinearArray<T>::empty() const
    {
        return DynLinearBuffer::empty();
    }
}