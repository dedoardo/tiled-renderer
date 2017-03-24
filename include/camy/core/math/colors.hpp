/* colors.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/core/math/types.hpp>

namespace camy
{
	namespace colors
	{
		inline const float4 from_packed(uint32 packed)
		{
			return { static_cast<float>((packed >> 24) & 0xFF) / 255,
				static_cast<float>((packed >> 16) & 0xFF) / 255,
				static_cast<float>((packed >> 8) & 0xFF) / 255,
				static_cast<float>((packed) & 0xFF) / 255 };
		}

		const float4 white =		from_packed(0xFFFFFFFF);
		const float4 black =		from_packed(0x000000FF);
		const float4 ivory =		from_packed(0xFFFFF0FF);
		const float4 beige =		from_packed(0xF5F5DCFF);
		const float4 wheat =		from_packed(0xF5DeB3FF);
		const float4 tan =			from_packed(0xD2B48CFF);
		const float4 khaki =		from_packed(0xC3B091FF);
		const float4 silver =		from_packed(0xC0C0C0FF);
		const float4 gray =			from_packed(0x808080FF);
		const float4 charcoal =		from_packed(0x464646FF);
		const float4 navy_blue =	from_packed(0x000080FF);
		const float4 royal_blue =	from_packed(0x084C9EFF);
		const float4 medium_blue =	from_packed(0x0000CDFF);
		const float4 azure =		from_packed(0x007FFFFF);
		const float4 cyan =			from_packed(0x00FFFFFF);
		const float4 aquamarine =	from_packed(0x7FFFD4FF);
		const float4 teal =			from_packed(0x008080FF);
		const float4 forest_green = from_packed(0x228B22FF);
		const float4 olive =		from_packed(0x808000FF);
		const float4 chartreuse =	from_packed(0x7FFF00FF);
		const float4 lime =			from_packed(0xBFFF00FF);
		const float4 golden =		from_packed(0xFFD700FF);
		const float4 golden_rod =	from_packed(0xDAA520FF);
		const float4 coral =		from_packed(0xFF7F50FF);
		const float4 salmon =		from_packed(0xFA8072FF);
		const float4 hot_pink =		from_packed(0xFC9FC9FF);
		const float4 fuchsia =		from_packed(0xFF77FFFF);
		const float4 puce =			from_packed(0xCC8899FF);
		const float4 mauve =		from_packed(0xE0B0FFFF);
		const float4 lavendere =	from_packed(0xB57EDCFF);
		const float4 plum =			from_packed(0x843179FF);
		const float4 indigo =		from_packed(0x4B0082FF);
		const float4 maroon =		from_packed(0x800000FF);
		const float4 crimson =		from_packed(0xDC143CFF);
	}
}