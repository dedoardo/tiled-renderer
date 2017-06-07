/* byte_vector.hpp
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
    // Untyped raw linear allocator.
    // Memory is contiguous and allocation = pointer bumping
    class CAMY_API DynLinearBuffer
    {
    public:
        static const rsize DEFAULT_CAPACITY = 1024;

    public:
        explicit DynLinearBuffer(rsize capacity = DEFAULT_CAPACITY,
                                 rsize alignment = DEFAULT_ALIGNMENT);
        ~DynLinearBuffer();

        DynLinearBuffer(const DynLinearBuffer& other);
        DynLinearBuffer(DynLinearBuffer&& other);

        DynLinearBuffer& operator=(const DynLinearBuffer& other);
        DynLinearBuffer& operator=(DynLinearBuffer&& other);

        // Returns current pointer.
        //! This is NOT the last element, but the byte right after it
        byte* cur();
        const byte* cur() const;

        // Returns base pointer
        byte* data();
        const byte* data() const;

        // Bumps pointer ahead by bytes bytes
        //! The returned pointer is guaranteed to be writable for the first bytes bytes
        byte* incr(rsize bytes);

        // Bumps pointer ahead by bytes bytes and writes bytes from src
        void write(const byte* src, rsize bytes);

        // Resets the current pointer to base
        void clear();

        rsize count() const;
        rsize capacity() const;
        bool empty() const;

    private:
        byte* _allocate_align_explicit(rsize n, rsize alignment);
        byte* _allocate_align_same(rsize n, void* src_alignment);
        void _deallocate(byte* ptr);
        void _make_space(rsize n);

        byte* m_beg;
        byte* m_cur;
        byte* m_end;
    };
}