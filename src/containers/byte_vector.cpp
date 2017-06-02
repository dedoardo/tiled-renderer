/* byte_vector.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/containers/byte_vector.hpp>

namespace camy
{
    ByteVector::ByteVector() : m_buffer(nullptr), m_size(0), m_capacity(0)
    {
        m_capacity = 256;
        m_buffer = (byte*)API::allocate(CAMY_UALLOC(m_capacity));
        m_size = 0;
    }

    ByteVector::~ByteVector() { API::deallocate(m_buffer); }

    ByteVector::ByteVector(ByteVector&& other)
    {
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_size = 0;
    }

    ByteVector::ByteVector(const ByteVector& other)
    {
        m_buffer = (byte*)API::allocate(CAMY_UALLOC(other.m_capacity));
        memcpy(m_buffer, other.m_buffer, other.m_capacity);
        m_capacity = other.m_capacity;
        m_buffer = other.m_buffer;
    }

    ByteVector& ByteVector::operator=(ByteVector&& other)
    {
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_size = 0;
        return *this;
    }

    ByteVector& ByteVector::operator=(const ByteVector& other)
    {
        m_buffer = (byte*)API::allocate(CAMY_UALLOC(other.m_capacity));
        memcpy(m_buffer, other.m_buffer, other.m_capacity);
        m_capacity = other.m_capacity;
        m_buffer = other.m_buffer;
        return *this;
    }

    byte* ByteVector::data() { return m_buffer; }

    void ByteVector::clear() { m_size = 0; }

    rsize ByteVector::count() { return m_size; }
}