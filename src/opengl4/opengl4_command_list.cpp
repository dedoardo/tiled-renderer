// Header
#include <camy/command_list.hpp>

#if defined(CAMY_BACKEND_OPENGL4)

// camy
#include <camy/render_context.hpp>
#include <camy/opengl4/opengl4_command_list.hpp>

#define new_cmd(name, idx)\
struct name { static const ::camy::rsize kVal = idx; };

namespace camy
{
	namespace OpenGL4 { namespace Cmd
	{
		void(*cmd_ftbl[])(RenderContextData&, CommandListData&, const byte*&) =
		{
			cmd_BindProgramPipeline,
			cmd_BindFramebuffer,
			cmd_BindInputSignature,
			cmd_Enable,
			cmd_Disable,
			cmd_PolygonMode,
			cmd_CullFace,
			cmd_PolygonOffsetClamp,
			cmd_DepthFunc,
			cmd_Viewport,
			cmd_DepthRange,
			cmd_BindBuffer,
			cmd_BindBufferBase,
			cmd_BindBufferRange,
			cmd_ActiveTexture,
			cmd_BindTexture,
			cmd_BindSampler,
			cmd_BindVertexArray,
			cmd_BindVertexBuffer,
			cmd_DrawArrays,
			cmd_DrawElementsBaseVertex,
			cmd_DrawArraysInstancedBaseInstance,
			cmd_DrawElementsInstancedBaseVertex,
			cmd_ClearTexImage
		}; 
	}}

	using namespace OpenGL4;

	CommandList::CommandList()
	{

	}

	CommandList::~CommandList()
	{

	}

	void CommandList::begin()
	{
		m_data.command_buffer.clear();
	}

	void CommandList::end()
	{

	}

	void CommandList::clear_color(HResource handle, const float4& color)
	{
		Surface& surface = API::rc().get_surface(handle);

		Cmd::append<Cmd::ClearTexImage>(m_data,
			surface.native.texture, 
			GLint(0), 
			(GLenum)GL_RGBA, 
			(GLenum)GL_FLOAT, 
			(uint8)sizeof(float4),color);
	}

	void CommandList::clear_depth(HResource handle, float depth)
	{
		Surface& surface = API::rc().get_surface(handle);

		Cmd::append<Cmd::ClearTexImage>(m_data,
			surface.native.texture, 
			GLint(0), 
			(GLenum)GL_RGBA, 
			(GLenum)GL_FLOAT, 
			(uint8)sizeof(float), 
			depth);
	}
	
	void CommandList::clear_stencil(HResource handle, uint32 val)
	{
		Surface& surface = API::rc().get_surface(handle);

		Cmd::append<Cmd::ClearTexImage>(m_data,
			surface.native.texture, 
			GLint(0), 
			(GLenum)GL_RGBA, 
			(GLenum)GL_FLOAT, 
			(uint8)sizeof(uint32), 
			val);
	}

	void CommandList::set_vertex_shader(HResource handle)
	{
		m_data.cur_vertex_shader = handle;
	}

	void CommandList::set_geometry_shader(HResource handle)
	{
		// TODO
	}

	void CommandList::set_pixel_shader(HResource handle)
	{
		m_data.cur_pixel_shader = handle;
	}

	CAMY_INLINE uint64 hash_targets(const HResource* render_targets, rsize num_render_targets, HResource depth_buffer)
	{
		static_assert(sizeof(HResource) == 2, "Hashing based on this assumption, rewrite if it changes");
		uint64 hash = 0x0;
		rsize i = 0;
		while (i < num_render_targets)
		{
			uint64 partial = 0;
			rsize j = 0;
			while (j < 4 && i < num_render_targets)
			{
				partial |= ((uint64)render_targets[i]._v & 0xFF) << (j * 16);
				++j; ++i;
			}

			hash ^= partial;
		}
		
		return hash ^= depth_buffer;
	}

	void CommandList::set_targets(const HResource* render_targets, rsize num_render_targets, HResource depth_buffer, uint8* views)
	{
		if (render_targets == nullptr && num_render_targets > 0)
		{
			CL_ERR("Invalid argument: render_targets is null or empty");
			return;
		}

		if (num_render_targets > API::MAX_RENDER_TARGETS)
		{
			CL_ERR("Invalid argument: num_render_targets must be between [0, ", API::MAX_RENDER_TARGETS, "(API::MAX_RENDER_TARGETS)]");
			return;
		}

		uint64 hash = hash_targets(render_targets, num_render_targets, depth_buffer);
		if (m_data.fbo_map[hash] == nullptr)
		{
			InitResRequest req;
			req.type = InitResRequest::Type::Framebuffer;
			static_assert(sizeof(HResource) == 2, "Code below relies on this assumption as HResource hasnt been defined yet");
			for (rsize i = 0; i < num_render_targets; ++i)
				req.framebuffer.render_targets[i] = (uint16&)render_targets[i];
			req.framebuffer.depth_buffer = (uint16&)depth_buffer;
			req.hash = hash;
			m_data.init_requests.append(req);
			m_data.fbo_map.insert(hash, 0);
		}

		Cmd::append<Cmd::BindFramebuffer>(m_data, (GLenum)GL_FRAMEBUFFER, hash);
	}

	GLenum camy_to_opengl(PrimitiveTopology topology)
	{
		switch (topology)
		{
		case PrimitiveTopology::PointList:
			return GL_POINTS;
		case PrimitiveTopology::LineList:
			return GL_LINES;
		case PrimitiveTopology::LineStrip:
			return GL_LINE_STRIP;
		case PrimitiveTopology::TriangleList:
			return GL_TRIANGLES;
		case PrimitiveTopology::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		default:
			return GL_TRIANGLES;
		}
	}

	void CommandList::set_primitive_topology(PrimitiveTopology topology)
	{
		m_data.cur_primitive_topology = camy_to_opengl(topology);
	}

	void CommandList::set_rasterizer_state(HResource handle)
	{
		if (handle.is_invalid())
			return;

		RasterizerState rs = API::rc().get_rasterizer_state(handle);

		Cmd::append<Cmd::PolygonMode>(m_data, (GLenum)GL_FRONT_AND_BACK,
			(GLenum)(rs.desc.fill == RasterizerStateDesc::Fill::Solid ? GL_FILL : GL_LINE));

		if (rs.desc.cull == RasterizerStateDesc::Cull::None)
			Cmd::append<Cmd::Disable, GLenum>(m_data, GL_CULL_FACE);
		else
		{
			Cmd::append<Cmd::Enable, GLenum>(m_data, GL_CULL_FACE);
			Cmd::append<Cmd::CullFace, GLenum>(m_data,
				rs.desc.cull == RasterizerStateDesc::Cull::Front ? GL_FRONT : GL_BACK);
		}


		// factor = slopeScaleDepthBias
		// units = depthBias
		Cmd::append<Cmd::PolygonOffsetClamp>(m_data,
			(float)rs.desc.slope_scaled_depth_bias, 
			(float)rs.desc.depth_bias, 
			(float)rs.desc.depth_bias_clamp);
	}

	GLenum camy_to_opengl(DepthStencilStateDesc::DepthFunc func)
	{
		switch (func)
		{
		case DepthStencilStateDesc::DepthFunc::LessEqual:
			return GL_LEQUAL;
		case DepthStencilStateDesc::DepthFunc::Less:
		default:
			return GL_LESS; 
		}
	}

	void CommandList::set_depth_stencil_state(HResource handle)
	{
		if (handle.is_invalid())
			return;

		DepthStencilState& dss = API::rc().get_depth_stencil_state(handle);

		Cmd::append<Cmd::DepthFunc>(m_data, camy_to_opengl(dss.desc.depth_func));
	}

	void CommandList::set_input_signature(HResource handle)
	{
		Cmd::append<Cmd::BindInputSignature>(m_data, handle);
	}

	void CommandList::set_viewport(const Viewport& viewport)
	{
		Cmd::append<Cmd::Viewport>(m_data,
			(GLint)viewport.left, (GLint)viewport.top,
			(GLsizei)(viewport.right - viewport.left),
			(GLsizei)(viewport.bottom - viewport.top));

		Cmd::append<Cmd::DepthRange>(m_data,
			(GLfloat)viewport.near, 
			(GLfloat)viewport.far);
	}

	void CommandList::set_vertex_buffer(rsize slot, HResource handle)
	{
		m_data.cur_vertex_buffers[slot] = handle;
	}

	void CommandList::set_vertex_buffers(rsize slot, const HResource* handles, rsize num_handles)
	{
		// TODO: move to glBindVertexBuffers()
		for (rsize i = 0; i < num_handles; ++i)
			set_vertex_buffer(slot + i, handles[i]);
	}

	void CommandList::set_index_buffer(HResource handle)
	{
		m_data.cur_index_buffer = handle;
	}

	void CommandList::set_cbuffer(ShaderVariable var, HResource handle)
	{
		ConstantBuffer& cb = API::rc().get_constant_buffer(handle);

		Cmd::append<Cmd::BindBufferBase>(m_data,
			(GLenum)GL_UNIFORM_BUFFER, 
			(GLuint)var.slot(), 
			cb.native.buffer);
	}

	void CommandList::set_cbuffer_off(ShaderVariable var, HResource handle, rsize offset)
	{
		ConstantBuffer& cb = API::rc().get_constant_buffer(handle);
		GLintptr byte_offset = offset * API::query(API::Query::ConstantByteSize);
		
		Cmd::append<Cmd::BindBufferRange>(m_data,
			(GLenum)GL_UNIFORM_BUFFER, 
			(GLuint)var.slot(), 
			cb.native.buffer, 
			(GLintptr)byte_offset, 
			(GLsizeiptr)var.size());
	}

	void CommandList::set_parameter(ShaderVariable var, HResource handle, uint8 view)
	{
		switch ((BindType)var.type())
		{
		case BindType::Surface:
		{
			Surface& surface = API::rc().get_surface(handle);
			// TODO: Switch based on shader type
			Cmd::append<Cmd::ActiveTexture>(m_data, (GLenum)(GL_TEXTURE0 + var.slot()));
			Cmd::append<Cmd::BindTexture>(m_data, (GLenum)GL_TEXTURE_2D, surface.native.texture);
			break;
		}
		case BindType::Sampler:
		{
			Sampler& sampler = API::rc().get_sampler(handle);
			Cmd::append<Cmd::BindSampler>(m_data, (GLuint)var.slot(), sampler.native.sampler);
			break;
		}
		}
	}

	CAMY_INLINE void validate_ppo(CommandListData& data)
	{
		static_assert(sizeof(HResource) == 2, "Assumption for underlying code");
		uint64 hash = data.cur_vertex_shader;
		hash |= (uint64)data.cur_pixel_shader << 16;
		
		if (data.ppo_map[hash] == nullptr)
		{
			InitResRequest req;
			req.type = InitResRequest::Type::ProgramPipeline;
			req.program_pipeline.vertex_shader = data.cur_vertex_shader;
			req.program_pipeline.pixel_shader = data.cur_pixel_shader;
			req.hash = hash;
			data.init_requests.append(req);
			data.ppo_map.insert(hash, 0);
		}

		Cmd::append<Cmd::BindProgramPipeline>(data, hash);
	}

	CAMY_INLINE void validate_vao(CommandListData& data)
	{
		static_assert(sizeof(HResource) == 2, "Assumption for underlying code");

		// Binding VAO
		Cmd::append<Cmd::BindVertexArray>(data, (HResource)data.cur_input_signature);
		
		// Vertex buffers
		for (rsize i = 0; i < API::MAX_VERTEX_BUFFERS; ++i)
		{
			if (((HResource&)data.cur_vertex_buffers[i]).is_valid())
			{
				VertexBuffer& vb = API::rc().get_vertex_buffer((HResource)data.cur_vertex_buffers[i]);
				Cmd::append<Cmd::BindVertexBuffer>(data,
					(GLuint)i, 
					vb.native.buffer,
					(GLintptr)0,
					(GLintptr&)vb.desc.element_size);
			}
		}

		// Index buffer
		if (((HResource&)data.cur_index_buffer).is_valid())
		{
			IndexBuffer& ib = API::rc().get_index_buffer((HResource)data.cur_index_buffer);
			Cmd::append<Cmd::BindBuffer>(data, (GLenum)GL_ELEMENT_ARRAY_BUFFER, ib.native.buffer);
		}
	}

	CAMY_INLINE void validate_containers(CommandListData& data)
	{
		// Pipeline state object is a container with delayed creation ( not sharable ) 
		validate_ppo(data);
	
		// VAO is a container object thus has delayed creation, but at the same time it
		// is owned by the user (InputSignature). Doing a validation at draw time avoids 
		// the user to have to bind an input signature before a vertex buffer. 
		// Even when using DSA in order to call a glVertexArrayVertexBuffer the VertexArray
		// needs to be known, this can't happen if InputSignature is set later. 
		validate_vao(data);
	}

	void CommandList::draw(uint32 vertex_count, uint32 vertex_offset)
	{
		validate_containers(m_data);

		Cmd::append<Cmd::DrawArrays>(m_data,
			(GLenum)m_data.cur_primitive_topology, 
			(GLint)vertex_offset, 
			(GLsizei)vertex_count);
	}

	void CommandList::draw_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset)
	{
		validate_containers(m_data);

		if (((HResource&)m_data.cur_index_buffer).is_invalid())
		{
			// ERROR
			return;
		}

		IndexBuffer& ib = API::rc().get_index_buffer(m_data.cur_index_buffer);
		GLsizei index_size = ib.desc.extended32 ? sizeof(uint32) : sizeof(uint16);

		Cmd::append<Cmd::DrawElementsBaseVertex>(m_data,
			(GLenum)m_data.cur_primitive_topology, 
			(GLsizei)index_count,
			(GLenum)(ib.desc.extended32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT),
			(char8*)0 + index_offset * index_size,
			(GLint)vertex_offset
			);
	}

	void CommandList::draw_instanced(uint32 vertex_count, uint32 instance_count, uint32 vertex_offset, uint32 instance_offset)
	{
		validate_containers(m_data);

		Cmd::append<Cmd::DrawArraysInstancedBaseInstance>(m_data,
			(GLenum)m_data.cur_primitive_topology,
			(GLint)vertex_offset, 
			(GLsizei)vertex_count,
			(GLsizei)instance_count, 
			(GLuint)instance_offset);
	}

	void CommandList::draw_indexed_instanced(uint32 index_count, uint32 instance_count, uint32 index_offset, uint32 vertex_offset, uint32 instance_offset)
	{
		validate_containers(m_data);

		if (((HResource&)m_data.cur_index_buffer).is_invalid())
		{
			// ERROR
			return;
		}

		IndexBuffer& ib = API::rc().get_index_buffer(m_data.cur_index_buffer);
		GLsizei index_size = ib.desc.extended32 ? sizeof(uint32) : sizeof(uint16);

		Cmd::append<Cmd::DrawElementsInstancedBaseVertex>(m_data,
			(GLenum)m_data.cur_primitive_topology,
			(GLsizei)index_count,
			(GLenum)(ib.desc.extended32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT),
			(char8*)0 + index_offset * index_size,
			(GLsizei)instance_count,
			(GLint)vertex_offset);
	}

	void CommandList::queue_constant_buffer_update(HResource handle, const void* data, rsize bytes)
	{
		CAMY_ASSERT(data != nullptr);
		CAMY_ASSERT(bytes != 0 && bytes % 16 == 0);
		m_updates.append({ data, bytes, handle });
	}
}

#endif