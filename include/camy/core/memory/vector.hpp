/* vector.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/memory/alloc.hpp>

namespace camy
{
	template <typename ElementType, uint16 kAlignment = kDefaultAlignment>
	class camy_api Vector final 
	{
    public:
        static const rsize kElementSize = sizeof(ElementType);

    public:
		Vector(rsize initial_capacity = 1);
		~Vector();

        Vector(Vector&& other);
        Vector(const Vector& other);

        Vector<ElementType, kAlignment>& operator=(Vector&& other);
        Vector<ElementType, kAlignment>& operator=(const Vector& other);

        // Access
        const ElementType&  operator[](rsize idx)const;
		ElementType&        operator[](rsize idx);
        const ElementType&  first()const;
        ElementType&        first();
        const ElementType&  last()const;
        ElementType&        last();
        ElementType*        data();
        const ElementType*  data()const;

        // Add ops
        void append(const ElementType& el);
        void append(ElementType&& el);
        template <typename ...CtorArgs>
        void emplace(CtorArgs&& ...ctor_args);
        // not copy ctor. **default** constructor
        void resize(rsize size);

        // Remove ops
        void pop();
        void remove(rsize idx);
        void clear();
        
        // Query
        rsize count()const;
		rsize capacity()const;
        bool  empty()const;
	private:
		void _realloc();

		ElementType* m_buffer;
		rsize m_size;
		rsize m_capacity;
	};

    template<typename ElementType, uint16 kAlignment>
    inline Vector<ElementType, kAlignment>::Vector(rsize initial_capacity) :
        m_buffer(nullptr),
        m_size(0),
        m_capacity(0)
    {
        camy_assert(initial_capacity > 0);
        m_capacity = initial_capacity;

        m_buffer = (ElementType*)allocate(camy_loc, kElementSize, kAlignment);
    }

    template<typename ElementType, uint16 kAlignment>
    inline Vector<ElementType, kAlignment>::~Vector()
    {
        for (rsize i = 0; i < m_size; ++i)
            m_buffer[i].~ElementType();
        deallocate(m_buffer);
    }

    template<typename ElementType, uint16 kAlignment>
    inline Vector<ElementType, kAlignment>::Vector(Vector&& other)
    {
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_size = 0;
    }

    template<typename ElementType, uint16 kAlignment>
    inline Vector<ElementType, kAlignment>::Vector(const Vector & other)
    {
        m_buffer = (ElementType*)allocate(camy_loc, kElementSize * other.m_capacity, kAlignment);
        m_capacity = other.m_capacity;
        m_size = other.m_size;

        for (rsize i = 0; i < m_size; ++i)
            new (&m_buffer[i]) ElementType(other.m_buffer[i]);
    }

    template<typename ElementType, uint16 kAlignment>
    inline Vector<ElementType, kAlignment>& Vector<ElementType, kAlignment>::operator=(Vector && other)
    {
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_size = 0;
        return *this;
    }

    template<typename ElementType, uint16 kAlignment>
    inline Vector<ElementType, kAlignment>& Vector<ElementType, kAlignment>::operator=(const Vector & other)
    {
        m_buffer = (ElementType*)allocate(camy_loc, kElementSize * other.m_capacity, kAlignment);
        m_capacity = other.m_capacity;
        m_size = other.m_size;

        for (rsize i = 0; i < m_size; ++i)
            new (&m_buffer[i]) ElementType(other.m_buffer[i]);
        return *this;
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType & Vector<ElementType, kAlignment>::operator[](rsize idx) const
    {
        camy_assert(m_buffer != nullptr);
        camy_assert(idx < m_size);
        return m_buffer[idx];
    }

    template<typename ElementType, uint16 kAlignment>
    inline ElementType & Vector<ElementType, kAlignment>::operator[](rsize idx)
    {
        camy_assert(m_buffer != nullptr);
        camy_assert(idx < m_size);
        return m_buffer[idx];
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType & Vector<ElementType, kAlignment>::first() const
    {
        camy_assert(!empty());
        return m_buffer[0];
    }

    template<typename ElementType, uint16 kAlignment>
    inline ElementType & Vector<ElementType, kAlignment>::first()
    {
        camy_assert(!empty());
        return m_buffer[0];
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType & Vector<ElementType, kAlignment>::last() const
    {
        camy_assert(!empty());
        return m_buffer[m_size - 1];
    }

    template<typename ElementType, uint16 kAlignment>
    inline ElementType & Vector<ElementType, kAlignment>::last()
    {
        camy_assert(!empty());
        return m_buffer[m_size - 1];
    }

    template<typename ElementType, uint16 kAlignment>
    inline ElementType * Vector<ElementType, kAlignment>::data()
    {
        return m_buffer;
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType * Vector<ElementType, kAlignment>::data() const
    {
        return m_buffer;
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::append(const ElementType & el)
    {
        if (m_size >= m_capacity)
            _realloc();

        m_buffer[m_size++] = el;
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::append(ElementType && el)
    {
        if (m_size >= m_capacity)
            _realloc();

        // TODO: Move assignment or construction ? maybe construction makes more sense.
        new (&m_buffer[m_size++]) ElementType(std::forward<ElementType>(el));
    }

    template<typename ElementType, uint16 kAlignment>
    template<typename ...CtorArgs>
    inline void Vector<ElementType, kAlignment>::emplace(CtorArgs && ...ctor_args)
    {
        if (m_size >= m_capacity)
            _realloc();

        new (&m_buffer[m_size++]) ElementType(std::forward<CtorArgs>(ctor_args)...);
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::resize(rsize size)
    {
        if (size == m_size) return;
        if (size < m_size)
        {
            for (rsize i = size; i < m_size; ++i)
                m_buffer[i].~ElementType();
        }
        else if (size <= m_capacity)
        {
            for (rsize i = m_size; i < size; ++i)
                new (&m_buffer[i]) ElementType();
        }
        // Need to reallocate
        else
        {
            ElementType* old_buffer = m_buffer;
            rsize new_capacity = size;
            m_buffer = (ElementType*)allocate(camy_loc, kElementSize * new_capacity, kAlignment);
            for (rsize i = 0; i < m_capacity; ++i)
                new (&m_buffer[i]) ElementType(old_buffer[i]);
            for (rsize i = m_capacity; i < new_capacity; ++i)
                new (&m_buffer[i]) ElementType();
      
            deallocate(old_buffer);
            m_capacity = new_capacity;
        }
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::pop()
    {
        camy_assert(!empty());
        m_buffer[--m_size].~ElementType();
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::remove(rsize idx)
    {
        camy_assert(idx < m_size);
        m_buffer[idx].~ElementType();

        new (&m_buffer[idx]) ElementType(std::move(m_buffer[--m_size]));
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::clear()
    {
        for (rsize i = 0; i < m_size; ++i)
            m_buffer[i].~ElementType();

        m_size = 0;
    }

    template<typename ElementType, uint16 kAlignment>
    inline rsize Vector<ElementType, kAlignment>::count() const
    {
        return m_size;
    }

	template<typename ElementType, uint16 kAlignment>
	inline rsize Vector<ElementType, kAlignment>::capacity() const
	{
		return m_capacity;
	}

    template<typename ElementType, uint16 kAlignment>
    inline bool Vector<ElementType, kAlignment>::empty() const
    {
        return m_size == 0;
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Vector<ElementType, kAlignment>::_realloc()
    {
        ElementType* old_buffer = m_buffer;
        rsize new_capacity = m_capacity * 2;
        m_buffer = (ElementType*)allocate(camy_loc, kElementSize * new_capacity, kAlignment);
        for (rsize i = 0; i < m_capacity; ++i)
            new (&m_buffer[i]) ElementType(std::move(old_buffer[i]));

        deallocate(old_buffer);
        m_capacity = new_capacity;
    }
}