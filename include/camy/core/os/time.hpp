/* time.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>

/*! 
	Simple high-resolution clock with minimal interface
	Usage:
		TimeSlice a = timer_split();
		TimeSlice b = timer_split();
		float milliseconds_delta = timer_elapsed(b - a);

	Note: It's not recommended to rely on TimeSlice for anything 
	else that calculating elapsed time . calling timer_elapsed(timer_split()) 
	is not guaranteed to return anythign meaningful.
!*/
namespace camy
{
	using TimeSlice = sint64;

	//! Returns an integer identifying the current timepoint (in ticks usually)
	TimeSlice timer_split();

	//! Returns the time in millisecond elapsed in a specific delta.
	//! the slice passed should always be a different between different time points
	float	  timer_elapsed(TimeSlice slice);
}