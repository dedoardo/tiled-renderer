/* thread.hpp
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
	using ThreadID = uint64;
	using ThreadProc = bool(*)(void*);

	ThreadID launch_thread(ThreadProc proc, void* pdata);
	void     join_thread(ThreadID thread);
	bool	 is_thread_valid(ThreadID thread);
	ThreadID current_thread_id();
	void	 yield_current();
	void	 sleep_current(rsize milliseconds);

	/*!
		On x86 the instruction sequences load->load, load->store, store->store are 
		all guaranteed to not be reordered by the hardware, so only the compiler has 
		to be notified for these. 
	!*/
	void compiler_fence();
	
	//! On x86 a store->load can be reordered by hardware, it needs its own memory fence
	void memory_fence();
	
	uint32 compare_and_swap(uint32& data, uint32 expected, uint32 desired);
	uint32 atomic_swap(uint32& data, uint32 desired);
	uint32 atomic_fetch_add(uint32& data, uint32 addend);

}