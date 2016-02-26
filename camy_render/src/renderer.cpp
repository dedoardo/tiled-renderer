// Header
#include <camy_render/renderer.hpp>

// camy
#include <camy/gpu_backend.hpp>
#include <camy_render/camera.hpp>
#include <camy_render/scene.hpp>

// C++ STL
#include <algorithm>
#include <bitset>

// Shaders
#define BYTE camy::Byte
#include "shaders/forward_vs.hpp"
#include "shaders/forward_ps.hpp"
#include "shaders/depth_only_vs.hpp"

#include "shaders/passthrough_pp.hpp"
#undef BYTE

using namespace std::placeholders;

namespace camy
{
	Renderer::Renderer() :
		m_window_surface{ nullptr },
		m_offscreen_target{ nullptr },

		m_max_lights{ 0 },

		// Depths => culling | Sky => Forward pass
		// Culling and sky have no dependencies doesn't matter which one is executed first, still,
		// both have to be finished before the forward pass
		m_light_culling_layer{ RenderLayer::Order::Ordered, 1},
		m_sky_layer{ RenderLayer::Order::Ordered, 1, 1, 1 },
		m_scene_depth_layer{ RenderLayer::Order::Ordered, 0, 1 },
		m_light_depth_layer{ RenderLayer::Order::Ordered, 0, 1 },
		m_forward_layer{ RenderLayer::Order::Ordered, 2, 2 },
		m_pp_layer{ RenderLayer::Order::Ordered, 3 }
	{
		m_layer_dispatcher.add_layer(&m_light_culling_layer);
		m_layer_dispatcher.add_layer(&m_sky_layer);
		m_layer_dispatcher.add_layer(&m_scene_depth_layer);
		m_layer_dispatcher.add_layer(&m_light_depth_layer);
		m_layer_dispatcher.add_layer(&m_forward_layer);
		m_layer_dispatcher.add_layer(&m_pp_layer);
	}

	Renderer::~Renderer()
	{
		unload();
	}

	bool Renderer::load(Surface* window_surface, const float4x4& projection, u32 effects, u32 max_lights, u32 shadow_map_size)
	{
		unload();

		if (window_surface == nullptr)
		{
			camy_error("Invalid argument: window_surface != nullptr");
			return false;
		}

		if (shadow_map_size == 0 || max_lights == 0)
		{
			camy_error("Invalid argument: max_lights > 0 && shadow_map_size > 0");
			return false;
		}

		camy_info("Loading renderer: ");
		camy_info("> Effects enabled: ", std::bitset<6>(effects));
		camy_info("> Max lights: ", max_lights);
		camy_info("> Shadow map size: ", shadow_map_size);
		
		/*
			First off we need to create the offscreen render target where the scene will be rendered ready for
			postprocessing. If HDR is enabled we are going to use a 16bit per channel format. Currently there is
			no way to control the HDR format, an option will be eventually added
		*/
		m_window_surface = window_surface;
		m_max_lights = max_lights;
		m_effects = effects;

		Surface::Format offscreen_format{ Surface::Format::RGBA8Unorm };
		if (effects == PostProcessPipeline::Effects_HDR)
		{
			offscreen_format = PostProcessPipeline::hdr_format;
			camy_info("HDR enabled, offscreen format RGBA16Float");
		}

		m_offscreen_target = hidden::gpu.create_render_target(offscreen_format, window_surface->description.width, window_surface->description.height);
		if (m_offscreen_target == nullptr)
		{
			camy_warning("Error: Failed to create offscreen render target");
			unload();
			return false;
		}

		/*
			Scene depth is used for light culling, a view space depth from the camera perspective will be output
		*/
		if (!m_scene_depth_pass.load(window_surface->description.width, window_surface->description.height, true))
		{
			camy_error("Error: failed to load scene depth pass");
			unload();
			return false;
		}

		/*
			Compute shader culling pass that uses info from the previous pass
		*/
		if (!m_light_culling_pass.load(window_surface->description.width, window_surface->description.height))
		{
			camy_error("Error: Failed to load light culling pass");
			unload();
			return false;
		}

		/*
			Light depth generates the shadow map, currently it is a very trivial implementation
		*/
		if (!m_light_depth_pass.load(shadow_map_size, shadow_map_size, true))
		{
			camy_error("Error: Failed to load light depth pass");
			unload();
			return false;
		}

		Surface* output_surface{ m_offscreen_target };
		if (effects == PostProcessPipeline::Effects_None)
			output_surface = m_window_surface;

		/*
			Currently sky is on by default, when tonemapping and environment parameters will be introduced it will be fixed
		*/
		if (!m_sky_pass.load(output_surface))
		{
			camy_error("Error: Failed to load sky pass");
			unload();
			return false;
		}

		/*
			Forward pass is the actual rendering pass where geometry + material + lighting is rendered alltogether
		*/
		if (!m_forward_pass.load(output_surface, max_lights))
		{
			camy_error("Failed to create forward pass");
			unload();
			return false;
		}

		/*
			Specified resources that need to be unbound before moving onto the next, in order to avoid hazards
		*/
		m_forward_deps[0] = m_forward_pass.get_shadow_map_var();
		m_forward_deps[1] = m_forward_pass.get_light_indices_var();
		m_forward_deps[2] = m_forward_pass.get_light_grid_var();
		m_forward_deps[3] = m_forward_pass.get_shadow_map_view_var();

		if (!m_post_process_pipeline.load(m_offscreen_target, m_window_surface, effects))
		{
			camy_error("Error: Failed to load post processing pipeline");
			unload();
			return false;
		}
		
		// Creating layer from it only if there are some items
		if (effects != PostProcessPipeline::Effects_None)
			m_pp_layer.create(&m_post_process_pipeline.pp_items[0], static_cast<u32>(m_post_process_pipeline.pp_items.size()));

		camy_info("Succesfully loaded Renderer and post-processing pipeline");

		return true;
	}

	void Renderer::unload()
	{
		m_sky_pass.unload();
		m_scene_depth_pass.unload();
		m_light_depth_pass.unload();
		m_light_culling_pass.unload();
		m_forward_pass.unload();

		hidden::gpu.safe_dispose(m_offscreen_target);
	}

	void Renderer::render(Scene& scene, Camera& camera)
	{
		Viewport vp;
		vp.left = vp.top = 0.f;
		vp.right = static_cast<float>(m_window_surface->description.width);
		vp.bottom = static_cast<float>(m_window_surface->description.height);
		
		render(scene, camera, vp);
	}

	void Renderer::render(Scene& scene, Camera& camera, const Viewport& viewport)
	{
		using namespace DirectX;

		// This is the single-threaded implementation where we retrieve an array of objects from the scene
		// and process them one by one, inserting the respective draw calls in the two renderlayers
		SceneNode** nodes;
		u32 node_count;
		scene.retrieve_visible(camera, nodes, node_count);
		
		// sky and light culling DO NOT required to loop thorough all the nodes
		// thus are processed before
		_queue_sky(scene, camera, viewport);

		// Need to reset the passes with current shared information ( e.g. camera ) 
		float4x4 light_view, light_projection, light_vp;
		math::store(light_view, scene.compute_sun_view());
		math::store(light_projection, scene.compute_sun_projection());
		math::store(light_vp, math::mul(math::load(light_view), math::load(light_projection)));

		m_scene_depth_pass.pre(camera.get_view(), camera.get_projection());
		m_light_depth_pass.pre(light_view, light_projection);

		// Move all positions to float3
		m_forward_pass.pre(camera, light_view, light_projection, 
			m_light_depth_pass.get_depth_buffer(), m_light_depth_pass.get_render_target(),
			m_light_culling_pass.get_light_indices(), m_light_culling_pass.get_light_grid());

		// Begin queueing
		m_scene_depth_layer.begin();
		m_light_depth_layer.begin();
		m_forward_layer.begin();

		// Here more sophisticated culling could be introduced
		for (auto i{ 0u }; i < node_count; ++i)
		{
			const auto& node{ nodes[i] };

			if (node->get_type() == SceneNode::Type::Render)
			{
				auto render_node{ static_cast<RenderSceneNode*>(node) };

				// All the rendernodes cast light thus:
				for (auto r{ 0u }; r < render_node->renderables.size(); ++r)
				{
					auto sd_ri{ m_scene_depth_layer.create_render_item(0) };
					auto ld_ri{ m_light_depth_layer.create_render_item(0) };

					// Todo: Check pointer

					m_scene_depth_pass.prepare(render_node, r, *sd_ri);
					m_light_depth_pass.prepare(render_node, r, *ld_ri);

					// Todo: add transparent
					auto fo_ri{ m_forward_layer.create_render_item(0) };
					m_forward_pass.prepare(render_node, r, *fo_ri);

					fo_ri->cached_parameter_groups[1].parameter_group->parameters->shader_variable;
				}
			}
			else if (node->get_type() == SceneNode::Type::Light)
				m_forward_pass.add_light(static_cast<const LightSceneNode*>(node));
		}
	
		// Updating resources
 		m_forward_pass.post(m_light_culling_pass.get_light_indices(), m_light_culling_pass.get_light_grid());

		// We can't light cull before the needed resources have been updated correctly
		_queue_light_culling(scene, camera, viewport);

		m_scene_depth_layer.end(0, m_scene_depth_pass.get_shared_parameters());
		m_light_depth_layer.end(0, m_light_depth_pass.get_shared_parameters(), &m_light_depth_out, 1);
		m_forward_layer.end(0, m_forward_pass.get_shared_parameters(), m_forward_deps, 3);
		m_forward_layer.end(1, m_forward_pass.get_shared_parameters(), m_forward_deps, 3);
	}

	void Renderer::sync()
	{
		// Executing commands
		m_layer_dispatcher.dispatch();

		// Swapping buffers
		hidden::gpu.swap_buffers(m_window_surface);
	}

	void Renderer::_queue_sky(Scene& scene, Camera& camera, const Viewport& viewport)
	{
		m_sky_layer.begin();
		m_sky_pass.prepare_single(camera, *m_sky_layer.create_render_item(0));
		m_sky_layer.end(0, m_sky_pass.get_shared_parameter_group());
	}

	void Renderer::_queue_light_culling(Scene& scene, Camera& camera, const Viewport& viewport)
	{
		m_light_culling_layer.begin();
		m_light_culling_pass.prepare_single(m_forward_pass.get_light_buffer(), m_scene_depth_pass.get_render_target(), 
			camera.get_view(), camera.get_projection(), m_forward_pass.get_num_lights(), *m_light_culling_layer.create_compute_item());
		m_light_culling_layer.end();
	}
}