/* linear_vector.hpp
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
    /*
        Heap allocated arena allocator. Heap counterpart of linear_array.
    */
	template <typename ElementType, uint16 kAlignment = kDefaultAlignment>
	class camy_api LinearVector final
 	{
    public:
        static const rsize kElementSize = sizeof(ElementType);

	public:
		LinearVector(rsize initial_capacity = 2);
		~LinearVector();

        LinearVector(LinearVector&& other);
        LinearVector(const LinearVector& other);

        LinearVector<ElementType, kAlignment>& operator=(LinearVector&& other);
        LinearVector<ElementType, kAlignment>& operator=(const LinearVector& other);

        ElementType& operator[](rsize idx);
        const ElementType& operator[](rsize idx)const;

		ElementType& next();
		void         reset();
		ElementType* data(); // ptr to 0th item
        ElementType& top();
        const ElementType& top()const;

        rsize count()const;
        bool  empty()const;
        rsize capacity()const;
	private:
		ElementType* m_buffer;
		ElementType* m_cur;
		rsize m_capacity;
	};

	template<typename ElementType, uint16 kAlignment>
	inline LinearVector<ElementType, kAlignment>::LinearVector(rsize initial_capacity) :
        m_buffer(nullptr),
        m_cur(nullptr),
        m_capacity(0)
	{
        camy_assert(initial_capacity > 0);

        m_buffer = (ElementType*)allocate(camy_loc, kElementSize * initial_capacity, kAlignment);
        m_cur = m_buffer;
		m_capacity = initial_capacity;
	}

    template<typename ElementType, uint16 kAlignment>
	inline LinearVector<ElementType, kAlignment>::~LinearVector()
	{
		deallocate(m_buffer);
	}

    template<typename ElementType, uint16 kAlignment>
    inline LinearVector<ElementType, kAlignment>::LinearVector(LinearVector&& other)
    {
        m_buffer = other.m_buffer;
        m_cur = other.m_cur;
        m_capacity = other.m_capacity;
        other.m_buffer = nullptr;
        other.m_cur = nullptr;
        other.m_capacity = 0;
    }

    template<typename ElementType, uint16 kAlignment>
    inline LinearVector<ElementType, kAlignment>::LinearVector(const LinearVector & other)
    {
        m_buffer = (ElementType*)allocate(camy_loc, kElementSize * other.m_capacity, kAlignment);
        m_cur = m_buffer = other.count();
        m_capacity = other.m_capacity;
        
        for (rsize i = 0; i < m_capacity; ++i)
            m_buffer[i] = other.m_buffer[i];
    }

    template<typename ElementType, uint16 kAlignment>
    inline LinearVector<ElementType, kAlignment>& LinearVector<ElementType, kAlignment>::operator=(LinearVector && other)
    {
        m_buffer = other.m_buffer;
        m_cur = other.m_cur;
        m_capacity = other.m_capacity;
        other.m_buffer = nullptr;
        other.m_cur = nullptr;
        other.m_capacity = 0;

        return *this;
    }

    template<typename ElementType, uint16 kAlignment>
    inline LinearVector<ElementType, kAlignment>& LinearVector<ElementType, kAlignment>::operator=(const LinearVector & other)
    {
        m_buffer = (ElementType*)allocate(camy_loc, kElementSize * other.m_capacity, kAlignment);
        m_cur = m_buffer = other.count();
        m_capacity = other.m_capacity;

        for (rsize i = 0; i < m_capacity; ++i)
            new (&m_buffer[i]) ElementType(other.m_buffer[i]);
        return *this;
    }

    template<typename ElementType, uint16 kAlignment>
    inline ElementType& LinearVector<ElementType, kAlignment>::operator[](rsize idx)
    {
        assert(idx < count());
        return m_buffer[idx];
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType& LinearVector<ElementType, kAlignment>::operator[](rsize idx) const
    {
        assert(idx < count());
        return m_buffer[idx];
    }

    template<typename ElementType, uint16 kAlignment>
	inline ElementType& LinearVector<ElementType, kAlignment>::next()
	{
		camy_assert(m_buffer != nullptr);
		camy_assert(m_cur != nullptr);
		camy_assert(m_capacity > 0);

		// Realloc
		if (m_cur >= m_buffer + m_capacity)
		{
			ElementType* old_buffer = m_buffer;
			rsize new_capacity = m_capacity * 2;
            m_buffer = (ElementType*)allocate(camy_loc, kElementSize * new_capacity);

			m_cur = m_buffer;
			for (rsize i = 0; i < m_capacity; ++i)
                new (m_cur++) ElementType(std::move(old_buffer[i]));

            deallocate(old_buffer);
			m_capacity = new_capacity;
		}

		new (m_cur) ElementType();
		return *(m_cur++);
	}

    template<typename ElementType, uint16 kAlignment>
	inline void LinearVector<ElementType, kAlignment>::reset()
	{
		m_cur = m_buffer;
	}

    template<typename ElementType, uint16 kAlignment>
	inline ElementType * LinearVector<ElementType, kAlignment>::data()
	{
		return m_buffer;
	}

    template<typename ElementType, uint16 kAlignment>
    inline ElementType & LinearVector<ElementType, kAlignment>::top()
    {
        camy_assert(!empty());
        return *(m_cur - 1);
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType & LinearVector<ElementType, kAlignment>::top() const
    {
        camy_assert(!empty());
        return *(m_cur - 1);
    }

    template<typename ElementType, uint16 kAlignment>
	inline rsize LinearVector<ElementType, kAlignment>::count() const
	{
		return (rsize)(m_cur - m_buffer);
	}

    template<typename ElementType, uint16 kAlignment>
    inline bool LinearVector<ElementType, kAlignment>::empty() const
    {
        return count() == 0;
    }

    template<typename ElementType, uint16 kAlignment>
    inline rsize LinearVector<ElementType, kAlignment>::capacity() const
    {
        return m_capacity;
    }
}