/* thread.cpp
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

namespace camy
{
	namespace API
	{
		DWORD WINAPI thread_launcher(void* args)
		{
			bool ret = ((ThreadProc*)args)[0](((void**)args)[1]);
			deallocate(args);
			return ret ? 0 : 1;
		}

		ThreadID thread_launch(ThreadProc proc, void* pdata)
		{
			void** args = (void**)API::allocate(CAMY_UALLOC(sizeof(void*) * 2));
			args[0] = proc;
			args[1] = pdata;
			HANDLE thread_id;
			if (FAILED(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)thread_launcher, args, 0x0, (DWORD*)&thread_id)))
			{
				CL_ERR("Win32::CreateThread failed with error: ", GetLastError());
				return (ThreadID)INVALID_HANDLE_VALUE;
			}
			return (ThreadID)thread_id;
		}

		void thread_join(ThreadID thread)
		{
			if ((HANDLE)thread == INVALID_HANDLE_VALUE)
			{
				CL_ERR("Invalid argument: thread is invalid");
				return;
			}

			WaitForSingleObject((HANDLE)thread, INFINITE);
		}

		bool thread_is_valid(ThreadID thread)
		{
			return (HANDLE)thread != INVALID_HANDLE_VALUE;
		}

		ThreadID thread_current()
		{
			return (ThreadID)GetCurrentThread();
		}

		void thread_yield_current()
		{
			YieldProcessor();
		}

		void thread_sleep_current(rsize milliseconds)
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

		uint32 atomic_cas(uint32& data, uint32 expected, uint32 desired)
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

		Futex futex_create()
		{
			CRITICAL_SECTION* futex = API::tallocate<CRITICAL_SECTION>(CAMY_UALLOC1);
			InitializeCriticalSection(futex);
			return (void*)futex;
		}

		void futex_destroy(Futex futex)
		{
			if (futex == nullptr) return;
			DeleteCriticalSection((CRITICAL_SECTION*)futex);
			API::tdeallocate((CRITICAL_SECTION*&)futex);
		}

		void futex_lock(Futex futex)
		{
			if (futex == nullptr) return;
			EnterCriticalSection((CRITICAL_SECTION*)futex);
		}

		void futex_unlock(Futex futex)
		{
			if (futex == nullptr) return;
			LeaveCriticalSection((CRITICAL_SECTION*)futex);
		}

		void debug_break()
		{
			DebugBreak();
		}
	}
}

#else
#	error No implementation available for this platform
#endif