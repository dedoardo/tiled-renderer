#pragma once

// camy
#include "../base.hpp"
#include "page.hpp"

// C++ STL
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace camy
{
	namespace allocators
	{
		/*
			Class: PagedLinearAllocator
				This allocator is paged and linear, meaning that pointers won't be invalidated because it's being
				resized. Memory can't be deallocated individually, it's deallocated altogheter. It's especially
				useful to allocate a bunch of memory on a per-frame basis. If you need a non-paged linear allocator
				use a std::vector. 
				Note : memory is not guaranteed to be contiguous, that's why it's paged, as said above, if you need 
				contiguous memory simply go for a std::Vector.

				At first I inteded to make all the allocators thread free, but having one allocator per thread is
				not an huge overhead and avoids a lot of possible problems
		*/
		template <Size page_size>
		class PagedLinearAllocator final
		{
		public:
			/*
				Constructor: PagedLinearAllocator
					Creates a new instances of the allocator, preallocating a paged
			*/
			PagedLinearAllocator(u32 alignment = 16);

			/*
				Destructor: ~PagedLinearAllocator
					Releases all the memory associated with this allocator
			*/
			~PagedLinearAllocator();

			/*
				 Function: allocate
					Allocates a chunk of memory of the specified size, obviously no construcor is called
			*/
			void* allocate(Size size);

			/*
				Function: allocate<T>
					Allocates a chunk of memory corresponding to <Type>, constructor is called and parameters
					can be passed.
			*/
			template <typename Type, typename ...CtorArgs>
			Type* allocate(CtorArgs&&... ctor_args);

			/*
				Function: reset
					Resets the allocator invalidating all the previous pointer and allowing
					for reusing the memory  from the previous allocations. 
			*/
			void reset();

		private:
			u32	m_alignment;

			/*
				Var: m_current_page
					Current page we are allocating from, pages are double linked this way we don't need 
					an extra pointer to the head of the list. When resetting we have to  roll-back but 
					that's not a huge problem, afterall there won't be an excessive number of pages
			*/
			Page<page_size>* m_current_page;
		};
	}
}

#include "paged_linear_allocator.inl"