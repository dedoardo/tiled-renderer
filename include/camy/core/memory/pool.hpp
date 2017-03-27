/* pool.hpp
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
#include <camy/core/memory/stack.hpp>

namespace camy
{
    // TODO: Assigning pool doesnt call constructor, simple bitwise copy
    // that's why it is currently disabled
    template <typename ElementType, uint16 kAlignment = kDefaultAlignment>
    class Pool final
    {
    public:
        static const rsize kElementSize = sizeof(ElementType);

    public:
        Pool(rsize initial_capacity = 2);
        ~Pool();

        Pool(Pool&& other);
        Pool(Pool& other) = delete;

        Pool& operator=(Pool&& other);
        Pool& operator=(const Pool& other) = delete;
        
        rsize reserve();
        template <typename ...CtorArgs>
        rsize allocate(CtorArgs&& ...ctor_args);
        ElementType& get(rsize idx);
        const ElementType& get(rsize idx)const;

        void deallocate(rsize idx);
        void clear();
        rsize capacity()const;

    private:
        void _realloc();

        ElementType*  m_base;
        ElementType*  m_top;
        Stack<rsize>  m_freelist;
    };

    template<typename ElementType, uint16 kAlignment>
    inline Pool<ElementType, kAlignment>::Pool(rsize initial_capacity) :
        m_base(nullptr)
    {
        m_base = (ElementType*)::camy::allocate(camy_loc, kElementSize * initial_capacity, kAlignment);
        m_top = m_base + initial_capacity;
        for (rsize i = 0; i < initial_capacity; ++i)
            m_freelist.push(initial_capacity - i - 1);
    }

    template<typename ElementType, uint16 kAlignment>
    inline Pool<ElementType, kAlignment>::~Pool()
    {
        ::camy::deallocate(m_base);
    }

    template<typename ElementType, uint16 kAlignment>
    inline Pool<ElementType, kAlignment>::Pool(Pool && other)
    {
        m_base = other.m_base;
        m_top = other.m_top;
        m_freelist = std::move(other.m_freelist);
        other.m_base = nullptr;
        other.m_top = nullptr;
    }
    
    template<typename ElementType, uint16 kAlignment>
    inline Pool<ElementType, kAlignment>& Pool<ElementType, kAlignment>::operator=(Pool && other)
    {
        m_base = other.m_base;
        m_top = other.m_top;
        m_freelist = std::move(other.m_freelist);
        other.m_base = nullptr;
        other.m_top = nullptr;
        return *this;
    }

    template<typename ElementType, uint16 kAlignment>
    inline rsize Pool<ElementType, kAlignment>::reserve()
    {
        if (m_freelist.empty())
            _realloc();

        rsize ret = m_freelist.pop();
        new (m_base + ret) ElementType();
        return ret;
    }

    template<typename ElementType, uint16 kAlignment>
    template<typename ...CtorArgs>
    inline rsize Pool<ElementType, kAlignment>::allocate(CtorArgs && ...ctor_args)
    {
        rsize idx = reserve();
        new (m_base + idx) ElementType(std::forward<CtorArgs>(ctor_args)...);
        return idx;
    }

    template<typename ElementType, uint16 kAlignment>
    inline ElementType & Pool<ElementType, kAlignment>::get(rsize idx)
    {
        camy_assert(idx < capacity());
        return m_base[idx];
    }

    template<typename ElementType, uint16 kAlignment>
    inline const ElementType & Pool<ElementType, kAlignment>::get(rsize idx) const
    {
        camy_assert(idx < capacity());
        return m_base[idx];
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Pool<ElementType, kAlignment>::deallocate(rsize idx)
    {
        m_freelist.push(idx);
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Pool<ElementType, kAlignment>::clear()
    {
        m_freelist.clear();
        for (rsize i = 0; i < capacity(); ++i)
            m_freelist.push(capacity() - i - 1);
    }

    template<typename ElementType, uint16 kAlignment>
    inline rsize Pool<ElementType, kAlignment>::capacity() const
    {
        return (rsize)(m_top - m_base);
    }

    template<typename ElementType, uint16 kAlignment>
    inline void Pool<ElementType, kAlignment>::_realloc()
    {
        camy_assert(m_freelist.empty());
        ElementType* old_buffer = m_base;
        rsize old_capacity = capacity();
        rsize new_capacity = old_capacity * 2;
        m_base = (ElementType*)::camy::allocate(camy_loc, kElementSize * new_capacity, kAlignment);
        m_top = m_base + new_capacity;
        for (rsize i = 0; i < old_capacity; ++i)
        {
            new (m_base + i) ElementType(std::move(old_buffer[i]));
            m_freelist.push(new_capacity -i - 1);
        }
        ::camy::deallocate(old_buffer);
    }
}