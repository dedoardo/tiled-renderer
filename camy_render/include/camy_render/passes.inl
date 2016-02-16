// camy
#include <camy/camy_init.hpp>
#include <camy/camy_math.hpp>
#include <camy/gpu_backend.hpp>

// render
#include "camera.hpp"
#include "shader_common.hpp"

namespace camy
{
	camy_inline void DepthPass::prepare(const RenderSceneNode* render_node, u32 renderable_index, RenderItem& render_item_out)
	{
		render_item_out.vertex_buffer1 = render_node->vertex_buffer1;
		render_item_out.vertex_buffer2 = render_node->vertex_buffer2;
		render_item_out.index_buffer = render_node->index_buffer;
		render_item_out.draw_info = render_node->renderables[renderable_index].draw_info;

		// Setting pipeline states
		render_item_out.vertex_shader = &m_vertex_shader;
		render_item_out.pixel_shader = nullptr;
		if (m_output_view_as_rt)
			render_item_out.pixel_shader = &m_pixel_shader;
		render_item_out.common_states = &m_common_states;

		// Creating parameters
		auto data_parameter{ m_data_parameter_allocator.allocate<PipelineParameter>() };
		data_parameter->shader_variable = m_per_object_var;
		data_parameter->data = render_node->get_global_transform();

		auto param_group{ m_parameter_group_allocator.allocate<ParameterGroup>() };
		param_group->num_parameters = 1;
		param_group->parameters = data_parameter;

		// Setting parameters
		render_item_out.cached_parameter_groups[0].parameter_group = param_group;
		render_item_out.cached_parameter_groups[0].cache_slot = 0;
		render_item_out.num_cached_parameter_groups = 1;
	}
	
	camy_inline void LightCullingPass::prepare_single(const Buffer* lights_buffer, const Surface* view_rt, const float4x4& view, const float4x4& projection, u32 num_lights, ComputeItem& compute_item_out)
	{
		// Setting params
		m_culling_dispatch_args.num_lights = num_lights;
		math::store(m_culling_dispatch_args.projection, math::transpose(math::load (projection)));
		math::store(m_culling_dispatch_args.view, math::transpose(math::load(view)));

		m_parameters[4].data = lights_buffer;
		m_parameters[5].data = view_rt;

		// Resetting index ( There has to be a better way really, i also have to use UpdateSubResource )
		uint data{ 0 };
		hidden::gpu.update(m_next_light_index, &data);

		// Preparing item
		compute_item_out.compute_shader = &m_compute_shader;
		compute_item_out.group_countx = m_culling_dispatch_args.num_tiles.x;
		compute_item_out.group_county = m_culling_dispatch_args.num_tiles.y;
		compute_item_out.group_countz = 1;
		compute_item_out.key = 0;
		compute_item_out.parameters = m_parameter_group;
	}

	camy_inline void SkyPass::prepare_single(const Camera& camera, RenderItem& render_item_out)
	{
		// Updating WVP values
		math::store(m_per_frame_object.view_projection, math::transpose(math::load(camera.get_view_projection())));
		math::store(m_per_frame_object.world, math::transpose(math::create_translation(math::load(camera.get_position()))));

		render_item_out.vertex_buffer1 = m_vertex_buffer;
		render_item_out.vertex_buffer2 = nullptr;
		render_item_out.index_buffer = m_index_buffer;
		render_item_out.common_states = &m_common_states;
		render_item_out.draw_info.index_count = 36;
		render_item_out.draw_info.index_offset =
			render_item_out.draw_info.vertex_offset = 0;
		render_item_out.draw_info.primitive_topology = PrimitiveTopology::TriangleList;
		render_item_out.key = 0;
		render_item_out.vertex_shader = &m_vertex_shader;
		render_item_out.pixel_shader = &m_pixel_shader;
		render_item_out.geometry_shader = nullptr;

		render_item_out.num_cached_parameter_groups = 0;


		// Sky pass is the first to run thus it's the one clearing the target surface
		float clear_color[]{ 0.15f, 0.15f, 0.15f, 1.f };
		hidden::gpu.clear_surface(m_common_states.render_targets[0], clear_color, 1.f, 0);
	}

	camy_inline void ForwardPass::prepare(const RenderSceneNode* render_node, u32 renderable_index, RenderItem& render_item_out)
	{
		render_item_out.vertex_buffer1 = render_node->vertex_buffer1;
		render_item_out.vertex_buffer2 = render_node->vertex_buffer2;
		render_item_out.index_buffer = render_node->index_buffer;
		render_item_out.draw_info = render_node->renderables[renderable_index].draw_info;

		// Setting pipeline states
		render_item_out.vertex_shader = &m_vertex_shader;
		render_item_out.pixel_shader = &m_pixel_shader;
		render_item_out.common_states = &m_common_states;

		// Setting parameters
		// Material + World ( two different cache slots ) 
		auto data_params{ static_cast<PipelineParameter*>(m_parameters_allocator.allocate<PipelineParameter>()) };
		data_params->shader_variable = m_vertex_shader.get(shaders::PerObject::name);
		data_params->data = render_node->get_global_transform();
		
		auto world_param_group{ m_parameter_group_allocator.allocate<ParameterGroup>() };
		world_param_group->num_parameters = 1;
		world_param_group->parameters = data_params;

		PipelineParameter* material_params{ static_cast<PipelineParameter*>(m_parameters_allocator.allocate<PipelineParameter>()) };
		auto material_param_count{ 0u };
		const auto& renderable{ render_node->renderables[renderable_index] };
		
		
		if (renderable.material == nullptr)
		{
			camy_warning("Found renderable without material when rendering forward pass");
		}
		else
		{
			// Looking up textures and setting them as parameters, 
			// They are part of the same parameter group as the material data 
			auto map_count{ 0u };
			
			if (renderable.material->render_feature_set & shaders::RenderFeatureSet_ColorMap) ++map_count;
			if (renderable.material->render_feature_set & shaders::RenderFeatureSet_MetalnessMap) ++map_count;
			if (renderable.material->render_feature_set & shaders::RenderFeatureSet_SmoothnessMap) ++map_count;

			auto next_free{ 1u };
			// Allocating all the maps + the cbuffer
			// [0] is for the cbuffer, rest for maps
			material_params = static_cast<PipelineParameter*>(m_parameters_allocator.allocate(sizeof(PipelineParameter) * (map_count + 1)));
			material_param_count = map_count + 1;
			auto map_params{ material_params };

			if (renderable.material->render_feature_set & shaders::RenderFeatureSet_ColorMap)
			{
				material_params[next_free].shader_variable = m_pixel_shader.get(shaders::color_map_name);
				material_params[next_free].data = renderable.color_map;
			
				camy_test_if(material_params[next_free].data == nullptr, camy_warning("RenderFeatureSet_ColorMap is enabled, but the specified color map is null"));

				if (material_params[next_free].data != nullptr)
					++next_free;
			}
		
			if (renderable.material->render_feature_set & shaders::RenderFeatureSet_MetalnessMap)
			{
				material_params[next_free].shader_variable = m_pixel_shader.get(shaders::metalness_map_name);
				material_params[next_free].data = renderable.metalness_map;

				camy_test_if(material_params[next_free].data == nullptr, camy_warning("RenderFeatureSet_MetalnessMap is enabled, but the specified metalness map is null"));

				if (material_params[next_free].data != nullptr)
					++next_free;
			}

			if (renderable.material->render_feature_set & shaders::RenderFeatureSet_SmoothnessMap)
			{
				material_params[next_free].shader_variable = m_pixel_shader.get(shaders::smoothness_map_name);
				material_params[next_free].data = renderable.smoothness_map;

				camy_test_if(material_params[next_free].data == nullptr, camy_warning("RenderFeatureSet_SmoothnessMap is enabled, but the specified smoothness map is null"));

				if (material_params[next_free].data != nullptr)
					++next_free;
			}
		}

		material_params->shader_variable = m_pixel_shader.get(shaders::Material::name);
		material_params->data = render_node->renderables[renderable_index].material;

		auto material_param_group{ m_parameter_group_allocator.allocate<ParameterGroup>() };
		material_param_group->num_parameters = material_param_count;
		material_param_group->parameters = material_params;

		render_item_out.cached_parameter_groups[0].cache_slot = 0;
		render_item_out.cached_parameter_groups[0].parameter_group = world_param_group;

		render_item_out.cached_parameter_groups[1].cache_slot = 1;
		render_item_out.cached_parameter_groups[1].parameter_group = material_param_group;

		render_item_out.num_cached_parameter_groups = 2;
	}

	camy_inline void ForwardPass::add_light(const LightSceneNode* node)
	{
		camy_assert(m_next_light + 1 < m_max_lights, 
		{
			camy_warning("Setting too many lights, max is: ", m_max_lights);
			return;
		});

		m_light_data[m_next_light++] = node->get_light();
	}
}