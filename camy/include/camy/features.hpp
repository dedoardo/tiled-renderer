#pragma once

// camy
#include "camy_base.hpp"

namespace camy
{
	/*
		Namespace: features
			Most of the resource limits are listed here, they are not stricly related to the underlying API
	*/
	namespace features
	{
		const u32 max_bindable_vertex_buffers{ 32 };
		const u32 max_bindable_render_targets{ 8 };

		const u32 max_constant_buffer_size{ 1024 };

		const u32 max_bindable_constant_buffers{ 14 };
		const u32 max_bindable_samplers{ 16 };
		const u32 max_bindable_shader_resources{ 128 };

		// TODO : REMOVE
		const u32 max_cbuffer_size{ 4096 * 16 };

		const u32 max_cachable_rts{ 2 };
		const u32 max_cachable_vbs{ 2 };
		const u32 num_cache_slots{ 5 };
	}
}