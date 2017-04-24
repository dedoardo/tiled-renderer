/* thread.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/os/thread.hpp>

// camy
#include <camy/core/memory/alloc.hpp>

#if defined(camy_os_windows)
#include <Windows.h>

namespace camy
{
	DWORD WINAPI thread_launcher(void* args)
	{
		bool ret = ((ThreadProc*)args)[0](((void**)args)[1]);
		deallocate(args);
		return ret ? 0 : 1;
	}

	ThreadID launch_thread(ThreadProc proc, void* pdata)
	{
		void** args = (void**)allocate(camy_loc, sizeof(void*) * 2);
		args[0] = proc;
		args[1] = pdata;
		HANDLE thread_id;
		if (FAILED(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)thread_launcher, args, 0x0, (DWORD*)&thread_id)))
		{
			cl_system_err("Win32::CreateThread", GetLastError(), "");
			return (ThreadID)INVALID_HANDLE_VALUE;
		}
		return (ThreadID)thread_id;
	}
	
	void join_thread(ThreadID thread)
	{
		if ((HANDLE)thread == INVALID_HANDLE_VALUE)
		{
			cl_invalid_arg(thread);
			return;
		}

		WaitForSingleObject((HANDLE)thread, INFINITE);
	}

	bool is_thread_valid(ThreadID thread)
	{
		return (HANDLE)thread != INVALID_HANDLE_VALUE;
	}

	ThreadID current_thread_id()
	{
		return (ThreadID)GetCurrentThread();
	}

	void yield_current()
	{
		YieldProcessor();
	}

	void sleep_current(rsize milliseconds)
	{
		Sleep((DWORD)milliseconds);
	}

	void compiler_fence()
	{
		_ReadWriteBarrier();
	}

	void memory_fence()
	{
		MemoryBarrier();
	}

	uint32 compare_and_swap(uint32& data, uint32 expected, uint32 desired)
	{
		return (uint32)_InterlockedCompareExchange((long*)&data, desired, expected);
	}

	uint32 atomic_swap(uint32& data, uint32 desired)
	{
		return (uint32)_InterlockedExchange((long*)&data, desired);
	}
	
	uint32 atomic_fetch_add(uint32& data, uint32 addend)
	{
		return _InterlockedExchangeAdd((long*)&data, addend);
	}
}

#else
#	error Please implement threading for your platform
#endif