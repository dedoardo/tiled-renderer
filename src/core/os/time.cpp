/* time.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/core/os/time.hpp>

namespace camy
{
#if defined(_WIN32)
#include <Windows.h>

	TimeSlice timer_split()
	{
		LARGE_INTEGER ts;
		QueryPerformanceCounter(&ts);
		return (TimeSlice)ts.QuadPart;
	}

	float timer_elapsed(TimeSlice slice)
	{
		// TODO: Cache ?
		LARGE_INTEGER fq;
		QueryPerformanceFrequency(&fq);
		return (float)slice / ((float)fq.QuadPart / 1000.f);
	}
#endif
}