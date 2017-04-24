/* alloc.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>

namespace camy
{
	constexpr rsize kDefaultAlignment = 16;
	// All the following function work iff 'camy_enable_memory_tracking' is defined.
	namespace memtrack
	{
		// Starts tracking. 
		void start_tracking();

		// Returns the total number of bytes currently allocated and yet to be freed
		// if everything has been properly deallocated it returns 0.
		rsize total_allocated_bytes();

		// Iterates over the active allocations. This is useful to dump leaks at the end of an application
		// Timestamp contains the duration in milliseconds since start_tracking() was called.
		using AllocationFoundCallback = void(*)(const char8* file, uint16 line, rsize bytes, uint16 timestamp);
		void iterate_allocations(AllocationFoundCallback callback);
	}

	// C allocations -> no ctor/dtor called
	// C++ allocations -> ctor/dtor called w/ custom arguments
	// the first two parameters are used iff 'camy_enable_memory_tracking' is defined.

	// Instead of having full blown macro, simply using this at the beginning of an allocation routine
#define camy_loc __FILE__, (::camy::uint16)__LINE__

	// C Tracking allocation routines.  Power of 2 alignment
	void* allocate(const char8* file, uint16 line, rsize size, uint16 alignment = kDefaultAlignment);
	void  _deallocate(void*& ptr);
	template <typename T> void deallocate(T*& ptr) { _deallocate((void*&)ptr); }

	// Returns the number of allocated bytes for the pointer. The overhead is not included
	// this is the number passed to allocate()
	rsize  get_allocated_bytes(const void* ptr);

	// C++ allocation routines
	template <typename T, typename ...CtorArgs>
	T* tallocate(char8* file, uint16 line, uint32 alignment, CtorArgs&& ...ctor_args);
	template <typename T>
	void tdeallocate(T*& ptr);

	// C++ array allocation routines. ctor/dtor is called for **each** element
	template <typename T, typename ...CtorArgs>
	T* tallocate_array(const char8* file, uint16 line, rsize count, uint32 alignment, CtorArgs&& ...ctor_args);
	template <typename T>
	void tdeallocate_array(T*& ptr);


	// Implementation
	template<typename T, typename ...CtorArgs>
	T * tallocate(char8 * file, uint16 line, uint32 alignment, CtorArgs && ...ctor_args)
	{
		void* ptr = allocate(file, line, sizeof(T), alignment);
		new (ptr) T(std::forward<CtorArgs>(ctor_args)...);
		return (T*)ptr;
	}

	template<typename T>
	void tdeallocate(T*& ptr)
	{
		if (ptr == nullptr)
			return;
		ptr->~T();
		deallocate(ptr);
		ptr = nullptr;
	}

	template<typename T, typename ...CtorArgs>
	T* tallocate_array(const char8* file, uint16 line, rsize count, uint32 alignment, CtorArgs && ...ctor_args)
	{
		void* ptr = allocate(file, line, sizeof(T) * count, alignment);
		for (rsize i = 0; i < count; ++i)
			new ((T*)ptr + i) T(std::forward<CtorArgs>(ctor_args)...);
		return (T*)ptr;
	}

	template<typename T>
	void tdeallocate_array(T*& ptr)
	{
		if (ptr == nullptr)
			return;

		rsize bytes = get_allocated_bytes(ptr);
		camy_assert(bytes % sizeof(T) == 0);
		rsize count = bytes / sizeof(T);

		for (rsize i = 0; i < count; ++i)
			((T*)ptr + i)->~T();
		deallocate(ptr);
		ptr = nullptr;
	}
}