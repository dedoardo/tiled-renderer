// Header
#include <camy/graphics/command_list.hpp>

#if defined(camy_os_windows) && defined(camy_backend_opengl4)

namespace camy
{
	CommandList::CommandList()
	{

	}

	CommandList::~CommandList()
	{

	}

	void CommandList::begin()
	{
		
	}

	void CommandList::end()
	{

	}

	void CommandList::clear_color(HResource handle, const float4& color)
	{

	}

	void CommandList::clear_depth_stencil(HResource handle, float depth, uint stencil)
	{

	}

	void CommandList::set_vertex_shader(HResource handle)
	{

	}

	void CommandList::set_geometry_shader(HResource handle)
	{

	}

	void CommandList::set_pixel_shader(HResource handle)
	{

	}

	void CommandList::set_targets(const HResource* render_targets, rsize num_render_targets, HResource depth_buffer, uint8* views)
	{

	}

	void CommandList::set_primitive_topology()
	{

	}

	void CommandList::set_rasterizer_state(HResource handle)
	{

	}

	void CommandList::set_depth_stencil_state(HResource handle)
	{

	}

	void CommandList::set_input_signature(HResource handle)
	{

	}

	void CommandList::set_viewport(const Viewport & viewport)
	{

	}

	void CommandList::set_vertex_buffer(rsize slot, HResource handle)
	{

	}

	void CommandList::set_vertex_buffers(rsize slot, const HResource* handles, rsize num_handles)
	{

	}

	void CommandList::set_index_buffer(HResource handle)
	{

	}

	void CommandList::set_cbuffer(ShaderVariable var, HResource handle)
	{

	}

	void CommandList::set_cbuffer_off(ShaderVariable var, HResource handle, rsize offset)
	{
	}

	void CommandList::set_parameter(ShaderVariable var, HResource handle, uint8 view)
	{
	}

	void CommandList::draw(uint32 vertex_count, uint32 vertex_offset)
	{
	}

	void CommandList::draw_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset)
	{

	}

	void CommandList::draw_instanced(uint32 vertex_count, uint32 instance_count, uint32 vertex_offset, uint32 instance_offset)
	{
	}

	void CommandList::draw_indexed_instanced(uint32 index_count, uint32 instance_count, uint32 index_offset, uint32 vertex_offset, uint32 instance_offset)
	{

	}

	void CommandList::queue_constant_buffer_update(HResource handle, const void* data, rsize bytes)
	{

	}
}

#endif