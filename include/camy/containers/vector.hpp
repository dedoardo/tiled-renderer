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
	template <typename ElementType, uint16 kAlignment = DEFAULT_ALIGNMENT>
	class CAMY_API Vector final 
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
		CAMY_ASSERT(initial_capacity > 0);
		m_capacity = initial_capacity;

		m_buffer = (ElementType*)allocate(CAMY_ALLOC(kElementSize, kAlignment));
	}

	template<typename ElementType, uint16 kAlignment>
	inline Vector<ElementType, kAlignment>::~Vector()
	{
		for (rsize i = 0; i < m_size; ++i)
			m_buffer[i].~ElementType();
		API::deallocate(m_buffer);
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
		m_buffer = (ElementType*)API::allocate(CAMY_ALLOC(kElementSize * other.m_capacity, kAlignment));
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
		m_buffer = (ElementType*)API::allocate(CAMY_ALLOC(kElementSize * other.m_capacity, kAlignment));
		m_capacity = other.m_capacity;
		m_size = other.m_size;

		for (rsize i = 0; i < m_size; ++i)
			new (&m_buffer[i]) ElementType(other.m_buffer[i]);
		return *this;
	}

	template<typename ElementType, uint16 kAlignment>
	inline const ElementType & Vector<ElementType, kAlignment>::operator[](rsize idx) const
	{
		CAMY_ASSERT(m_buffer != nullptr);
		CAMY_ASSERT(idx < m_size);
		return m_buffer[idx];
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType & Vector<ElementType, kAlignment>::operator[](rsize idx)
	{
		CAMY_ASSERT(m_buffer != nullptr);
		CAMY_ASSERT(idx < m_size);
		return m_buffer[idx];
	}

	template<typename ElementType, uint16 kAlignment>
	inline const ElementType & Vector<ElementType, kAlignment>::first() const
	{
		CAMY_ASSERT(!empty());
		return m_buffer[0];
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType & Vector<ElementType, kAlignment>::first()
	{
		CAMY_ASSERT(!empty());
		return m_buffer[0];
	}

	template<typename ElementType, uint16 kAlignment>
	inline const ElementType & Vector<ElementType, kAlignment>::last() const
	{
		CAMY_ASSERT(!empty());
		return m_buffer[m_size - 1];
	}

	template<typename ElementType, uint16 kAlignment>
	inline ElementType & Vector<ElementType, kAlignment>::last()
	{
		CAMY_ASSERT(!empty());
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
			m_size = size;
		}
		else if (size <= m_capacity)
		{
			for (rsize i = m_size; i < size; ++i)
				new (&m_buffer[i]) ElementType();
			m_size = size;
		}
		// Need to reallocate
		else
		{
			ElementType* old_buffer = m_buffer;
			rsize new_capacity = size;
			m_buffer = (ElementType*)allocate(CAMY_ALLOC(kElementSize * new_capacity, kAlignment));

			for (rsize i = 0; i < m_capacity; ++i)
				new (&m_buffer[i]) ElementType(std::move(old_buffer[i]));
			for (rsize i = m_capacity; i < new_capacity; ++i)
				new (&m_buffer[i]) ElementType();
	  
			API::deallocate(old_buffer);
			m_capacity = new_capacity;
			m_size = size;
		}
	}

	template<typename ElementType, uint16 kAlignment>
	inline void Vector<ElementType, kAlignment>::pop()
	{
		CAMY_ASSERT(!empty());
		m_buffer[--m_size].~ElementType();
	}

	template<typename ElementType, uint16 kAlignment>
	inline void Vector<ElementType, kAlignment>::remove(rsize idx)
	{
		CAMY_ASSERT(idx < m_size);
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
		m_buffer = (ElementType*)allocate(CAMY_ALLOC(kElementSize * new_capacity, kAlignment));

		for (rsize i = 0; i < m_capacity; ++i)
			new (&m_buffer[i]) ElementType(std::move(old_buffer[i]));

		API::deallocate(old_buffer);
		m_capacity = new_capacity;
	}
}