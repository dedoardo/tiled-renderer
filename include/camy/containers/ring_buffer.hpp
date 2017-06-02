/* ring_buffer.hpp
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
    /*
            Thread-safe single-producer, single-consumer ring buffer
    */
    template <typename ElementType>
    class CAMY_API RingBuffer final
    {
      public:
        RingBuffer(int pow2_capacity);
        ~RingBuffer();

        bool try_push(const ElementType& el);
        bool try_push(ElementType&& el);
        ElementType* try_pop();
        rsize capacity() const;
        rsize count() const;

      private:
        ElementType* m_buffer;
        rsize m_readpos;
        rsize m_writepos;
        rsize m_mask;
    };

    template <typename ElementType>
    RingBuffer<ElementType>::RingBuffer(int pow2_capacity)
    {
        rsize capacity = 1 << pow2_capacity;
        m_buffer = camy::API::tallocate_array<ElementType>(CAMY_UALLOC(capacity));
        m_readpos = m_writepos = 0;
        m_mask = capacity - 1;
    }

    template <typename ElementType>
    inline RingBuffer<ElementType>::~RingBuffer()
    {
        camy::API::tdeallocate_array(m_buffer);
    }

    template <typename ElementType>
    inline bool RingBuffer<ElementType>::try_push(const ElementType& el)
    {
        if (count() - capacity() == 0) return false;

        m_buffer[m_writepos & m_mask] = el;
        m_writepos atomic_fetch_add(m_writepos, 1);
        return true;
    }

    template <typename ElementType>
    inline bool RingBuffer<ElementType>::try_push(ElementType&& el)
    {
        if (count() - capacity() == 0) return false;

        m_buffer[m_writepos & m_mask] = std::forward<ElementType>(el);
        camy::API::atomic_fetch_add(m_writepos, 1);
        return true;
    }

    template <typename ElementType>
    inline ElementType* RingBuffer<ElementType>::try_pop()
    {
        if (m_readpos == m_writepos) return nullptr;
        ElementType* ret = &m_buffer[m_readpos & m_mask];
        camy::API::atomic_fetch_add(m_readpos, 1);
        return ret;
    }

    template <typename ElementType>
    inline rsize RingBuffer<ElementType>::capacity() const
    {
        return m_mask + 1;
    }

    template <typename ElementType>
    inline rsize RingBuffer<ElementType>::count() const
    {
        return m_writepos - m_readpos;
    }
}