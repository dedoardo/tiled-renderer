/* effect_bucket.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/graphics/base.hpp>
#include <camy/graphics/resource_manager.hpp>
#include <camy/graphics/render_bucket.hpp> // Stealing some structs from here

namespace camy
{
	//! Single Fullscreen postprocessing pass. 
	//! It's a friendlier and simpler version of a renderbucket where the 
	//! PipelineState is **not** shared among all items, but can be set every time.
	struct camy_api FSEffect
	{
		PipelineState pipeline_state;

		Parameter* parameters;
		rsize	   num_parameters;

		HResource vertex_shader;
		HResource pixel_shader;
	};
}