/* byte_vector.cpp
 *
 * Copyright (C) 2017 Edoardo Dominici
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
// Header
#include <camy/containers/dyn_linear_buffer.hpp>

namespace camy
{
	DynLinearBuffer::DynLinearBuffer(rsize capacity, rsize alignment)
	{
		m_beg = _allocate_align_explicit(capacity, alignment);
		m_cur = m_beg;
		m_end = m_beg + capacity;
	}

	DynLinearBuffer::~DynLinearBuffer()
	{
		_deallocate(m_beg);
	}

	DynLinearBuffer::DynLinearBuffer(const DynLinearBuffer& other)
	{
		m_beg = _allocate_align_same(other.capacity(), other.m_beg);
		m_cur = m_beg;
		m_end = m_beg + other.capacity();
	}

	DynLinearBuffer::DynLinearBuffer(DynLinearBuffer&& other) :
		m_beg(nullptr),
		m_cur(nullptr),
		m_end(nullptr)
	{
		API::swap(m_beg, other.m_beg);
		API::swap(m_cur, other.m_cur);
		API::swap(m_end, other.m_end);
	}

	DynLinearBuffer& DynLinearBuffer::operator=(const DynLinearBuffer& other)
	{
		_deallocate(m_beg);
		m_beg = m_cur = m_end = nullptr;
		m_beg = _allocate_align_same(other.capacity(), other.m_beg);
		m_cur = m_beg;
		m_end = m_beg + other.capacity();
		return *this;
	}

	DynLinearBuffer& DynLinearBuffer::operator=(DynLinearBuffer&& other)
	{
		_deallocate(m_beg);
		m_beg = m_cur = m_end = nullptr;
		API::swap(m_beg, other.m_beg);
		API::swap(m_cur, other.m_cur);
		API::swap(m_end, other.m_end);
		return *this;
	}

	byte* DynLinearBuffer::cur()
	{
		return m_cur;
	}

	const byte* DynLinearBuffer::cur()const
	{
		return m_cur;
	}

	byte* DynLinearBuffer::data()
	{
		return m_beg;
	}

	const byte* DynLinearBuffer::data() const
	{
		return m_beg;
	}

	byte* DynLinearBuffer::incr(rsize bytes)
	{
		_make_space(count() + bytes);
		m_cur += bytes;
		return m_cur - bytes;
	}
	
	void DynLinearBuffer::write(const byte* src, rsize bytes)
	{
		_make_space(count() + bytes);
		memcpy(m_cur, src, bytes);
		m_cur += bytes;
	}

	void DynLinearBuffer::clear()
	{
		m_cur = m_beg;
	}

	rsize DynLinearBuffer::count() const
	{
		return (rsize)(m_cur - m_beg);
	}

	rsize DynLinearBuffer::capacity() const
	{
		return (rsize)(m_end - m_beg);
	}

	bool DynLinearBuffer::empty() const
	{
		return m_cur == m_beg;
	}

	byte* DynLinearBuffer::_allocate_align_explicit(rsize n, rsize alignment)
	{
		return (byte*)API::allocate(CAMY_ALLOC(n, alignment));
	}

	byte* DynLinearBuffer::_allocate_align_same(rsize n, void* src_alignment)
	{
		return (byte*)API::allocate(CAMY_ALLOC_SRC(n, src_alignment));
	}

	void DynLinearBuffer::_deallocate(byte* ptr)
	{
		API::deallocate(ptr);
	}

	void DynLinearBuffer::_make_space(rsize n)
	{
		if (n > capacity())
		{
			byte* old_beg = m_beg;
			rsize old_capacity = capacity();
			rsize new_capacity = API::max(DEFAULT_CAPACITY, old_capacity * 2);
			m_beg = _allocate_align_same(new_capacity, old_beg);
			memcpy(m_beg, old_beg, old_capacity);
			_deallocate(old_beg);
			m_cur = m_beg + old_capacity;
			m_end = m_beg + new_capacity;
		}
	}
}