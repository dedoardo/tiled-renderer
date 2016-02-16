// Header
#include <camy_render/passes.hpp>

// camy
#include <camy/camy_init.hpp>
#include <camy/gpu_backend.hpp>

// Shaders
#define BYTE camy::Byte
#include "shaders/forward_vs.hpp"
#include "shaders/forward_ps.hpp"
#include "shaders/depth_only_vs.hpp"
#include "shaders/sky_vs.hpp"
#include "shaders/sky_ps.hpp"
#include "shaders/culling_cs.hpp"
#include "shaders/depth_view_vs.hpp"
#include "shaders/depth_view_ps.hpp" 
#undef BYTE

namespace camy
{
	//////////////////////////////////////////////////////////////////////////////
	////////////////////////////  DEPTH PASS /////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	DepthPass::DepthPass() :
		m_output_view_as_rt{ false }
	{

	}

	DepthPass::~DepthPass()
	{
		unload();
	}

	bool DepthPass::load(u32 target_width, u32 target_height, bool output_view_as_rt)
	{
		unload();

		// Creating Depth buffer ( needed in both scenarios ) it can be used as shader resource
		Surface::Description shadow_map_desc;
		shadow_map_desc.width = target_width;
		shadow_map_desc.height = target_height;
		shadow_map_desc.format = Surface::Format::R32Typeless;
		shadow_map_desc.format_dsv = Surface::Format::D32Float;
		shadow_map_desc.format_srv = Surface::Format::R32Float;

		m_common_states.depth_buffer = hidden::gpu.create_surface(shadow_map_desc, true, false, true);
		if (m_common_states.depth_buffer == nullptr)
		{
			unload();
			return false;
		}

		// Creating rasterizer state, kind of default for depth only pass ( backface culling, solid ) 
		m_common_states.rasterizer_state = hidden::gpu.create_rasterizer_state(RasterizerState::Cull::Back, RasterizerState::Fill::Solid);
		if (m_common_states.rasterizer_state == nullptr)
		{
			unload();
			return false;
		}

		if (output_view_as_rt)
		{
			// 16-bits can be enough, currently performance is no a major concern :) 
			m_common_states.render_targets[0] = hidden::gpu.create_render_target(Surface::Format::R32Float, target_width, target_height);
			if (m_common_states.render_targets[0] == nullptr)
			{
				unload();
				return false;
			}
		
			if (!m_vertex_shader.load(Shader::Type::Vertex, depth_view_vs, sizeof(depth_view_vs)))
			{
				unload();
				return false;
			}

			if (!m_pixel_shader.load(Shader::Type::Pixel, depth_view_ps, sizeof(depth_view_ps)))
			{
				unload();
				return false;
			}
		}
		else
		{
			if (!m_vertex_shader.load(Shader::Type::Vertex, depth_only_vs, sizeof(depth_only_vs)))
			{
				unload();
				return false;
			}
		}

		// Setting final states
		m_common_states.viewport.left =
			m_common_states.viewport.top = 0.f;
		m_common_states.viewport.right = static_cast<float>(target_width);
		m_common_states.viewport.bottom = static_cast<float>(target_height);

		// Parameters
		// Both vertex shaders have the same variable
		m_per_object_var = m_vertex_shader.get(shaders::PerObject::name);

		if (output_view_as_rt)
		{
			m_data_parameter.shader_variable = m_vertex_shader.get(shaders::PerFrameView::name);
			m_data_parameter.data = &m_per_frame_view_data;
		}
		else
		{
			m_data_parameter.shader_variable = m_vertex_shader.get(shaders::PerFrame::name);
			m_data_parameter.data = &m_per_frame_data;
		}

		// Warning should be issued by Shader::get, but that's pretty generic, giving more info here
		camy_test_if(m_data_parameter.shader_variable.valid == 0,
			camy_warning("Failed to retrieve PerFrame/View cbuffer for depth pass"));

		m_parameter_group.num_parameters = 1;
		m_parameter_group.parameters = &m_data_parameter;

		m_output_view_as_rt = output_view_as_rt;

		return true;
	}

	void DepthPass::unload()
	{
		hidden::gpu.safe_dispose(m_common_states.render_targets[0]);
		hidden::gpu.safe_dispose(m_common_states.depth_buffer);
		hidden::gpu.safe_dispose(m_common_states.rasterizer_state);
	}

	void DepthPass::pre(const float4x4& view, const float4x4& projection)
	{
		// Setting matrices
		if (m_output_view_as_rt)
		{
			math::store(m_per_frame_view_data.view, math::transpose(math::load(view)));
			math::store(m_per_frame_view_data.projection, math::transpose(math::load(projection)));
		}
		else
		{
			math::store(m_per_frame_data.view_projection, math::transpose(math::mul(math::load(view), math::load(projection))));
		}
		
		// Resetting allocators
		m_data_parameter_allocator.reset();
		m_parameter_group_allocator.reset();
		
		// Clearing buffers
		if (m_output_view_as_rt)
		{
			float max = std::numeric_limits<float>::max();
			float depth_view_clear[]{ max, max, max, max };
			hidden::gpu.clear_surface(m_common_states.render_targets[0], depth_view_clear, 1.f, 0u);
		}

		hidden::gpu.clear_surface(m_common_states.depth_buffer, nullptr, 1.f, 0u);
	}

	void DepthPass::post()
	{
		// Currently nothing to do, here for consistency
	}

	const Surface* DepthPass::get_depth_buffer()const
	{
		if (m_common_states.depth_buffer == nullptr)
			camy_warning("Trying to retrieve depth buffer from non-correctly initialize depth pass");
		return m_common_states.depth_buffer;
	}

	const Surface* DepthPass::get_render_target()const
	{
		if (m_common_states.render_targets[0] == nullptr)
			camy_warning("Trying to retrieve render target from non-correctly initialize depth pass or initialized but output_view_as_rt false");
		return m_common_states.render_targets[0];
	}

	bool DepthPass::is_output_view_as_rt_enabled()const
	{
		return m_output_view_as_rt;
	}

	const ParameterGroup* DepthPass::get_shared_parameters()const
	{
		return &m_parameter_group;
	}

	//////////////////////////////////////////////////////////////////////////////
	///////////////////// LIGHT CULLING PASS /////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	LightCullingPass::LightCullingPass() : 
		m_next_light_index{ nullptr },
		m_light_indices{ nullptr },
		m_light_grid{ nullptr }
	{

	}

	LightCullingPass::~LightCullingPass()
	{
		unload();
	}

	bool LightCullingPass::load(u32 target_width, u32 target_height)
	{
		unload();

		// Preparing shaders
		if (!m_compute_shader.load(Shader::Type::Compute, culling_cs, sizeof(culling_cs)))
		{
			unload();
			return false;
		}

		uint3 total_thread_count;
		m_culling_dispatch_args.num_tiles.x = static_cast<u32>(std::ceil(static_cast<float>(target_width) / camy_tile_size));
		m_culling_dispatch_args.num_tiles.y = static_cast<u32>(std::ceil(static_cast<float>(target_height) / camy_tile_size));
		m_culling_dispatch_args.num_tiles.z = 1;
		m_culling_dispatch_args.near = 0.1f;
		m_culling_dispatch_args.far = 100.f;
		m_culling_dispatch_args.width = static_cast<float>(target_width);
		m_culling_dispatch_args.height = static_cast<float>(target_height);


		m_next_light_index = hidden::gpu.create_buffer(Buffer::Type::Structured, 1, sizeof(uint), true);
		m_light_indices = hidden::gpu.create_buffer(Buffer::Type::Structured, camy_average_num_lights * m_culling_dispatch_args.num_tiles.x  * m_culling_dispatch_args.num_tiles.y,
			sizeof(uint), true);
		m_light_grid = hidden::gpu.create_buffer(Buffer::Type::Structured, m_culling_dispatch_args.num_tiles.x * m_culling_dispatch_args.num_tiles.y, sizeof(uint2), true);

		if (m_next_light_index == nullptr || m_light_indices == nullptr || m_light_grid == nullptr)
		{
			unload();
			return false;
		}

		// Preparing arguments
		m_parameters[0].shader_variable = m_compute_shader.get(shaders::CullingDispatchArgs::name);
		m_parameters[0].data = &m_culling_dispatch_args;

		m_parameters[1].shader_variable = m_compute_shader.get(shaders::next_light_index_name);
		m_parameters[1].data = m_next_light_index;
		m_parameters[2].shader_variable = m_compute_shader.get(shaders::light_indices_name);
		m_parameters[2].data = m_light_indices;
		m_parameters[3].shader_variable = m_compute_shader.get(shaders::light_grid_name);
		m_parameters[3].data = m_light_grid;
		m_parameters[4].shader_variable = m_compute_shader.get(shaders::lights_name);

		m_parameters[5].shader_variable = m_compute_shader.get(shaders::depth_map_name);

		m_parameter_group.num_parameters = 6;
		m_parameter_group.parameters = m_parameters;

		camy_info("Light culling loaded: [", m_culling_dispatch_args.num_tiles.x, "|", m_culling_dispatch_args.num_tiles.y, "|", 
			m_culling_dispatch_args.num_tiles.z, "]");

		return true;
	}

	void LightCullingPass::unload()
	{
		m_compute_shader.unload();
		hidden::gpu.safe_dispose(m_next_light_index);
		hidden::gpu.safe_dispose(m_light_indices);
		hidden::gpu.safe_dispose(m_light_grid);
	}

	const Buffer* LightCullingPass::get_light_indices()const
	{
		if (m_light_indices == nullptr)
			camy_warning("Trying to retrieve light indices from non-correctly initialized light culling pass");
		return m_light_indices;
	}

	ShaderVariable LightCullingPass::get_light_indices_var()const
	{
		auto ret { m_compute_shader.get(shaders::light_indices_name) };

		if (ret.valid == 0)
			camy_warning("Trying to retrieve light indices var from non-correctly initialized light culling pass");

		return ret;
	}

	const Buffer* LightCullingPass::get_light_grid()const
	{
		if (m_light_grid == nullptr)
			camy_warning("Trying to retreive light grid from non-correctly initialized light culling pass");
		return m_light_grid;
	}

	ShaderVariable LightCullingPass::get_light_grid_var()const
	{
		auto ret{ m_compute_shader.get(shaders::light_grid_name) };

		if (ret.valid == 0)
			camy_warning("Trying to retrieve light grid var from non-correctly initialized light culling pass");

		return ret;
	}

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// SKY PASS /////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	SkyPass::SkyPass() :
		m_vertex_buffer{ nullptr },
		m_index_buffer{ nullptr }
	{

	}

	SkyPass::~SkyPass()
	{
		unload();
	}

	bool SkyPass::load(Surface* target_surface)
	{
		unload();
	
		if (!m_vertex_shader.load(Shader::Type::Vertex, sky_vs, sizeof(sky_vs)))
		{
			unload();
			return false;
		}

		if (!m_pixel_shader.load(Shader::Type::Pixel, sky_ps, sizeof(sky_ps)))
		{
			unload();
			return false;
		}

		float3 vertices[8];

		vertices[0] = float3(-0.5f, 0.5f, -0.5f);
		vertices[1] = float3(-0.5f, 0.5f, 0.5f);
		vertices[2] = float3(0.5f, 0.5f, 0.5f);
		vertices[3] = float3(0.5f, 0.5f, -0.5f);

		//Bottom
		vertices[4] = float3(-0.5f, -0.5f, -0.5f);
		vertices[5] = float3(-0.5f, -0.5f, 0.5f);
		vertices[6] = float3(0.5f, -0.5f, 0.5f);
		vertices[7] = float3(0.5f, -0.5f, -0.5f);

		m_vertex_buffer = hidden::gpu.create_vertex_buffer(sizeof(float3), 8, vertices);
		if (m_vertex_buffer == nullptr)
		{
			unload();
			return false;
		}

		// Loading index buffer
		u16 indices[36];

		// Top
		indices[0] = 0; indices[1] = 1; indices[2] = 2;
		indices[3] = 0; indices[4] = 2; indices[5] = 3;

		// Bottom
		indices[6] = 4; indices[7] = 5; indices[8] = 6;
		indices[9] = 4; indices[10] = 6; indices[11] = 7;

		// Left
		indices[12] = 4; indices[13] = 5; indices[14] = 1;
		indices[15] = 4; indices[16] = 1; indices[17] = 0;

		// Right
		indices[18] = 6; indices[19] = 7; indices[20] = 3;
		indices[21] = 6; indices[22] = 3; indices[23] = 2;

		// Front
		indices[24] = 4; indices[25] = 0; indices[26] = 3;
		indices[27] = 4; indices[28] = 3; indices[29] = 7;

		// Back
		indices[30] = 6; indices[31] = 2; indices[32] = 1;
		indices[33] = 6; indices[34] = 1; indices[35] = 5;

		m_index_buffer = hidden::gpu.create_index_buffer(IndexBuffer::Type::U16, 36, indices);
		if (m_index_buffer == nullptr)
		{
			unload();
			return false;
		}

		m_common_states.viewport.left =
			m_common_states.viewport.top = 0.f;
		m_common_states.viewport.right = static_cast<float>(target_surface->description.width);
		m_common_states.viewport.bottom = static_cast<float>(target_surface->description.height);

		m_common_states.rasterizer_state = hidden::gpu.create_rasterizer_state(RasterizerState::Cull::None, RasterizerState::Fill::Solid);
		if (m_common_states.rasterizer_state == nullptr)
		{
			unload();
			return false;
		}

		m_common_states.render_targets[0] = target_surface;

		m_data_parameter.shader_variable = m_vertex_shader.get("PerFrameAndObject");
		m_data_parameter.data = &m_per_frame_object;

		m_parameter_group.num_parameters = 1;
		m_parameter_group.parameters = &m_data_parameter;

		return true;
	}

	void SkyPass::unload()
	{
		hidden::gpu.safe_dispose(m_vertex_buffer);
		hidden::gpu.safe_dispose(m_index_buffer);
		hidden::gpu.safe_dispose(m_common_states.rasterizer_state);
	}

	const ParameterGroup* SkyPass::get_shared_parameter_group()const
	{
		return &m_parameter_group;
	}

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////// FORWARD PASS /////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	ForwardPass::ForwardPass() :
		m_next_light{ 0 },
		m_light_data{ nullptr },
		m_light_buffer{ nullptr }
	{
		 
	}

	ForwardPass::~ForwardPass()
	{
		unload();
	}

	bool ForwardPass::load(Surface* target_surface, const u32 max_lights)
	{
		unload();

		m_max_lights = max_lights;
		m_light_data = new shaders::Light[max_lights];

		if (!m_vertex_shader.load(Shader::Type::Vertex, forward_vs, sizeof(forward_vs)))
		{
			unload();
			return false;
		}

		if (!m_pixel_shader.load(Shader::Type::Pixel, forward_ps, sizeof(forward_ps)))
		{
			unload();
			return false;
		}

		// Preparing common states
		m_common_states.render_targets[0] = target_surface;
		
		m_common_states.viewport.left =
			m_common_states.viewport.top = 0.f;
		m_common_states.viewport.right = static_cast<float>(target_surface->description.width);
		m_common_states.viewport.bottom = static_cast<float>(target_surface->description.height);

		m_common_states.depth_buffer = hidden::gpu.create_depth_buffer(Surface::Format::D24UNorm_S8Uint, target_surface->description.width, target_surface->description.height, target_surface->description.msaa_level);
		if (m_common_states.depth_buffer == nullptr)
		{
			unload();
			return false;
		}

		m_common_states.rasterizer_state = hidden::gpu.create_rasterizer_state(RasterizerState::Cull::Back, RasterizerState::Fill::Solid);

		m_parameters[0].shader_variable = m_pixel_shader.get(shaders::comparison_sampler_name);
		m_parameters[1].shader_variable = m_pixel_shader.get(shaders::default_sampler_name);

		m_parameters[0].data = hidden::gpu.create_sampler(Sampler::Filter::Linear, Sampler::Address::Mirror, Sampler::Comparison::Less);
		if (m_parameters[0].data == nullptr)
		{
			unload();
			return false;
		}

		m_parameters[1].data = hidden::gpu.create_sampler(Sampler::Filter::Linear, Sampler::Address::Wrap);
		if (m_parameters[1].data == nullptr)
		{
			unload();
			return false;
		}

		m_light_buffer = hidden::gpu.create_buffer(Buffer::Type::Structured, max_lights, sizeof(shaders::Light));
		if (m_light_buffer == nullptr)
		{
			unload();
			return false;
		}

		// Creating shared parameters
		m_parameters[2].shader_variable = m_vertex_shader.get(shaders::PerFrameLight::name);
		m_parameters[2].data = &m_per_frame;

		m_parameters[3].shader_variable = m_pixel_shader.get(shaders::Environment::name);
		m_parameters[3].data = &m_environment;

		m_environment.ambient_factor = 0.01f;
		m_environment.intensity = 1.f;
		//m_environment.screen_info = 0;
		m_environment.width = static_cast<float>(target_surface->description.width);
		m_environment.height = static_cast<float>(target_surface->description.height);
		m_environment.eye_position = float3_default;

		m_parameters[4].shader_variable = m_pixel_shader.get(shaders::shadow_map_name);
		m_parameters[4].data = nullptr;

		m_parameters[5].shader_variable = m_pixel_shader.get(shaders::lights_name);
		m_parameters[5].data = m_light_buffer;

		m_parameters[6].shader_variable = m_pixel_shader.get(shaders::light_indices_name);
		m_parameters[7].shader_variable = m_pixel_shader.get(shaders::light_grid_name);

		m_parameter_group.num_parameters = 2 + 2 + 1 + 3;
		m_parameter_group.parameters = m_parameters;

		return true;
	}

	void ForwardPass::unload()
	{
		hidden::gpu.safe_dispose(m_common_states.depth_buffer);

		safe_release_array(m_light_buffer);
	}

	void ForwardPass::pre(const Camera& camera, const float4x4& light_view, const float4x4& light_projection, const Surface* shadow_map, const Surface* shadow_map_view, const Buffer* light_indices, const Buffer* light_grid)
	{
		math::store(m_per_frame.view_projection, math::transpose(math::load(camera.get_view_projection())));
		math::store(m_per_frame.view_projection_light, math::transpose(math::mul(math::load(light_view), math::load(light_projection))));
		math::store(m_per_frame.view_light, math::transpose(math::load(light_view)));

		m_parameters[4].data = shadow_map;
		//m_parameters[5].data = shadow_map_view;

		m_environment.eye_position = camera.get_position();
		m_environment.light_direction = float3(-1.f, -1.f, -1.f);
		m_environment.near = camera.get_near_z();
		m_environment.far = camera.get_far_z();
		m_next_light = 0;

		m_parameter_group_allocator.reset();
		m_parameters_allocator.reset();

		// Clearing depth buffer, 
		// render target is previously cleared by the 
		hidden::gpu.clear_surface(m_common_states.depth_buffer, nullptr, 1.f, 0);
	}

	void ForwardPass::post(const Buffer* light_indices, const Buffer* light_grid)
	{
		// Updating light data
		hidden::gpu.update(m_light_buffer, m_light_data);

		m_parameters[6].data = light_indices;
		m_parameters[7].data = light_grid;
	}

	const ParameterGroup* ForwardPass::get_shared_parameters()const
	{
		return &m_parameter_group;
	}

	const Buffer* ForwardPass::get_light_buffer()const
	{
		if (m_light_buffer == nullptr)
			camy_warning("Trying to retrieve light buffer before the forward pass has been properly initialized");
		return m_light_buffer;
	}

	ShaderVariable ForwardPass::get_shadow_map_var()const
	{
		auto ret{ m_pixel_shader.get(shaders::shadow_map_name) };
		if (ret.valid == 0)
			camy_warning("Trying to retrieve shadow map before the forward pass has been properly initialized");
		return ret;
	}

	ShaderVariable ForwardPass::get_shadow_map_view_var()const
	{
		auto ret{ m_pixel_shader.get(shaders::shadow_map_view_name) };
		if (ret.valid == 0)
			camy_warning("Trying to retrieve shadow map view before the forward pass has been properly initialized");
		return ret;
	}

	ShaderVariable ForwardPass::get_light_indices_var()const
	{
		auto ret{ m_pixel_shader.get(shaders::light_indices_name) };
		if (ret.valid == 0)
			camy_warning("Trying to retrieve light indices before the forward pass has been properly initialized");
		return ret;
	}

	ShaderVariable ForwardPass::get_light_grid_var()const
	{
		auto ret{ m_pixel_shader.get(shaders::light_grid_name) };
		if (ret.valid == 0)
			camy_warning("Trying to retrieve light grid before the forward pass has been properly initialized");
		return ret;
	}

	u32 ForwardPass::get_num_lights() const
	{
		return m_next_light;
	}
}