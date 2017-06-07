/* alloc.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/

// Header
#include <camy/system.hpp>

#if defined(CAMY_OS_WINDOWS)
#include <Windows.h>
#endif

namespace camy
{
    namespace API
    {
        namespace
        {
#if defined(CAMY_ENABLE_MEMORY_TRACKING)
#pragma pack(push, 1)
            struct AllocationHeader
            {
                AllocationHeader* prev;
                AllocationHeader* next;
                const char8* filename;
                rsize bytes;
                rsize padding;
				rsize alignment;
				float timestamp;
				uint32 line;
            };
#pragma pack(pop)
#else
            // This is needed for tdeallocate_array that needs to query how many items there are
            // in order to call a destructor on them all
            struct AllocationHeader
            {
                rsize  bytes;
				rsize  alignment;
            };
#endif

            constexpr rsize ALLOCATION_HEADER_SIZE = sizeof(AllocationHeader);
// Thanks to Github.com/c4stan
#if defined(CAMY_OS_WINDOWS)
            CRITICAL_SECTION g_lock;
#endif
            TimeSlice g_start;
            sint64 g_total_bytes;
            AllocationHeader* g_head;
        }

        void memory_init()
        {
#if defined(CAMY_ENABLE_MEMORY_TRACKING)
            g_start = API::timer_split();
            g_total_bytes = 0;
#if defined(CAMY_OS_WINDOWS)
            InitializeCriticalSection(&g_lock);
#else
#error Implement
#endif
#endif
        }

        sint64 memory_total_bytes()
        {
#if defined(CAMY_ENABLE_MEMORY_TRACKING)
            return g_total_bytes;
#else
            return (rsize)-1;
#endif
        }

        rsize memory_block_size(const void* ptr)
        {
            if (ptr == nullptr) return 0;

            AllocationHeader* hdr = (AllocationHeader*)((byte*)ptr - ALLOCATION_HEADER_SIZE);
            return hdr->bytes;
        }

		rsize memory_block_alignment(const void* ptr)
		{
			if (ptr == nullptr) return 0;

			AllocationHeader* hdr = (AllocationHeader*)((byte*)ptr - ALLOCATION_HEADER_SIZE);
			return hdr->alignment;
		}

        void memory_iterate_allocations(AllocationFoundCallback callback)
        {
#if defined(CAMY_ENABLE_MEMORY_TRACKING)

            API::futex_lock(&g_lock);

            AllocationHeader* cur = g_head;
            while (cur != nullptr)
            {
                callback(cur->filename, cur->line, cur->bytes, cur->timestamp);
                cur = cur->next;
            }

            API::futex_unlock(&g_lock);
#endif
        }

        void* allocate(_AllocationInfo& info)
        {
            if (info.alignment == DEFAULT_ALIGNMENT) info.alignment = 16;

            // pow2
            CAMY_ASSERT(info.alignment > 0 && ((info.alignment & (info.alignment - 1)) == 0));

            // Lazy header allocation
            rsize bytes_to_allocate = info.count + ALLOCATION_HEADER_SIZE + info.alignment;
            byte* udata = (byte*)malloc(bytes_to_allocate) + ALLOCATION_HEADER_SIZE;
            byte* data_start = (byte*)((intptr_t)((byte*)udata + info.alignment) &
                                       ~(intptr_t)(info.alignment - 1));
            rsize padding = (rsize)(data_start - udata);

            //-----------------------
            API::futex_lock(&g_lock);
            AllocationHeader* hdr = (AllocationHeader*)(data_start - ALLOCATION_HEADER_SIZE);

#if defined(CAMY_ENABLE_MEMORY_TRACKING)
            hdr->prev = nullptr;
            hdr->next = nullptr;
            hdr->filename = info.file;
            hdr->bytes = info.count;
            hdr->padding = padding;
			hdr->alignment = info.alignment;
            hdr->timestamp = API::timer_elapsed(API::timer_split() - g_start);
			hdr->line = info.line;
#else
            hdr->bytes = info.count;
			hdr->alignment = info.alignment;
#endif
            g_total_bytes += (sint64)info.count;

            if (g_head == nullptr)
                g_head = hdr;
            else
            {
                ((AllocationHeader*)g_head)->prev = hdr;
                hdr->next = (AllocationHeader*)g_head;
                g_head = hdr;
            }
            API::futex_unlock(&g_lock);
            //-------------------------

            return data_start;
        }

        void _deallocate(void*& ptr)
        {
            if (ptr == nullptr) return;

            //----------------------
            API::futex_lock(&g_lock);

            AllocationHeader* hdr = (AllocationHeader*)((byte*)ptr - ALLOCATION_HEADER_SIZE);

            if (g_head == hdr) g_head = hdr->next;

            if (hdr->next != nullptr) hdr->next->prev = hdr->prev;

            if (hdr->prev != nullptr) hdr->prev->next = hdr->next;

            byte* udata = (byte*)hdr - hdr->padding;
            g_total_bytes -= (sint64)hdr->bytes;
            free(udata);

            CAMY_ASSERT(g_total_bytes >= 0);

            API::futex_unlock(&g_lock);
            //-------------------------
        }
    }
}