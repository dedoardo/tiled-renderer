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

namespace camy
{
	using TimeSlice = sint64;
	TimeSlice timer_split();
	float	  timer_elapsed(TimeSlice slice);
}