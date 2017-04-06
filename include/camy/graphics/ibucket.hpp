/* ibucket.hpp
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
	class CommandList;

	class camy_api IBucket
	{
	public:
		virtual ~IBucket() = default;

		// compiled version of the bucket
		virtual CommandList& to_command_list() = 0;
		virtual bool         is_ready()const = 0;
	}; 
}