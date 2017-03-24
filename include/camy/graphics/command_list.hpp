/* command_list.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/graphics/base.hpp>
#include <camy/graphics/resource_manager.hpp>
#include <camy/core/math/types.hpp>

namespace camy
{
	/*
		Class: CommandList
			Precompiled list of commands. Implementation depends on the backend.
			For instance on D3D11 with deferred context the CommandLists will be
			actual ID3D11CommandLists. On Opengl or D3D11 w/o deferred it will be
			a precompiled list of commands ( optimized for faster flushing ). 
	
			CommandList is generated from a bucket after items have been rendered
	*/
	class camy_api CommandList final
	{
	public:
		CommandList();
		~CommandList();

        void begin();
        void end();

        void clear_color(HResource handle, const float4& color);
        void clear_depth_stencil(HResource handle, float depth, uint stencil);

        void set_vertex_shader(HResource handle);
        void set_geometry_shader(HResource handle);
        void set_pixel_shader(HResource handle);
        void set_targets(const HResource* render_targets, rsize num_render_targets, HResource depth_buffer);

        void set_primitive_topology();
		void set_rasterizer_state(HResource handle);

        void set_input_signature(HResource handle);
        void set_viewport(const Viewport& viewport);
        void set_vertex_buffer(rsize slot, HResource handle);
        void set_vertex_buffers(rsize slot, const HResource* handles, rsize num_handles);
        void set_index_buffer(HResource handle);

        void set_parameter(ShaderVariable var, const void* data, HResource handle, rsize offset);
        void set_parameter(ShaderVariable var, HResource handle);

        void draw(uint32 vertex_count, uint32 vertex_offset);
        void draw_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset);
        void draw_instanced(uint32 vertex_count, uint32 instance_count, uint32 vertex_offset, uint32 instance_offset);
        void draw_indexed_instanced(uint32 index_count, uint32 instance_count, uint32 index_offset, uint32 vertex_offset, uint32 instance_offset);

        void queue_constant_buffer_update(HResource handle, const void* data, rsize bytes);

	private:
		friend class RenderContext;
		CommandListData m_data;
        
        struct ConstantBufferUpdate
        {
            const void* data;
            rsize bytes;
            HResource handle;
        };
        Vector<ConstantBufferUpdate> m_updates;
	};
}