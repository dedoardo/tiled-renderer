/* alloc.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/

// Header
#include <camy/core/memory/alloc.hpp>

// C++ STL
#include <mutex>
#include <chrono>

namespace camy
{
#if defined(camy_enable_memory_tracking)
#pragma pack(push, 1)
    struct AllocationHeader
    {
        AllocationHeader* prev;
        AllocationHeader* next;
        const char8* filename;
        rsize bytes;
        rsize padding;
        uint16 line;
        uint16 timestamp;
    };
#pragma pack(pop)
#else
    // Needed for c++ typed deallocations
    struct AllocationHeader
    {
        size bytes;
    };
#endif
    static const rsize kAllocationHeaderSize = sizeof(AllocationHeader);

    namespace memtrack
    {
        namespace
        {
            std::mutex g_mutex;
            std::chrono::high_resolution_clock::time_point g_start;
            rsize g_total_bytes;
            AllocationHeader* g_head;
        }

        void start_tracking()
        {
#if defined(camy_enable_memory_tracking)
            g_start = std::chrono::high_resolution_clock::now();
            g_total_bytes = 0;
#endif
        }

        rsize total_allocated_bytes()
        {
#if defined(camy_enable_memory_tracking)
            return g_total_bytes;
#else
            return (rsize)-1;
#endif
        }

        void iterate_allocations(AllocationFoundCallback callback)
        {
#if defined(camy_enable_memory_tracking)

            g_mutex.lock();

            AllocationHeader* cur = memtrack::g_head;
            while (cur != nullptr)
            {
                callback(cur->filename, cur->line, cur->bytes, cur->timestamp);
                cur = cur->next;
            }

            g_mutex.unlock();
#endif
        }
    }

    void* allocate(const char8* file, uint16 line, rsize size, uint16 alignment)
    {
        // pow2
        camy_assert(alignment > 0 && ((alignment & (alignment - 1)) == 0));
        
        // Lazy header allocation
        rsize bytes_to_allocate = size + kAllocationHeaderSize + alignment;
        byte* udata = (byte*)malloc(bytes_to_allocate) + kAllocationHeaderSize;
        byte* data_start = (byte*)((pointer_size)((byte*)udata + alignment) & ~(pointer_size)(alignment - 1));
        rsize padding = (rsize)(data_start - udata);

        //-----------------------
        memtrack::g_mutex.lock();
        AllocationHeader* hdr = (AllocationHeader*)(data_start - kAllocationHeaderSize);

#if defined(camy_enable_memory_tracking)
        hdr->prev = nullptr;
        hdr->next = nullptr;
        hdr->filename = file;
        hdr->bytes = size;
        hdr->padding = padding;
        hdr->line = line;
        hdr->timestamp = (uint16)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - memtrack::g_start).count());
#else
        hdr->bytes = size;
#endif
        memtrack::g_total_bytes += size;

        if (memtrack::g_head == nullptr)
            memtrack::g_head = hdr;
        else
        {
            ((AllocationHeader*)memtrack::g_head)->prev = hdr;
            hdr->next = (AllocationHeader*)memtrack::g_head;
            memtrack::g_head = hdr;
        }
        memtrack::g_mutex.unlock();
        //-------------------------

        return data_start;
    }

    void _deallocate(void*& ptr)
    {
        if (ptr == nullptr)
            return;
        
        //----------------------
        memtrack::g_mutex.lock();

        AllocationHeader* hdr = (AllocationHeader*)((byte*)ptr - kAllocationHeaderSize);

        if (memtrack::g_head == hdr)
            memtrack::g_head = hdr->next;

        if (hdr->next != nullptr)
            hdr->next->prev = hdr->prev;

        if (hdr->prev != nullptr)
            hdr->prev->next = hdr->next;

        byte* udata = (byte*)hdr - hdr->padding;        
        memtrack::g_total_bytes -= hdr->bytes;
		free(udata);

        camy_assert(memtrack::g_total_bytes >= 0);

        //-------------------------
        memtrack::g_mutex.unlock();
    }

    rsize get_allocated_bytes(const void * ptr)
    {
        if (ptr == nullptr)
            return 0;

        AllocationHeader* hdr = (AllocationHeader*)((byte*)ptr - kAllocationHeaderSize);
        return hdr->bytes;
    }
}