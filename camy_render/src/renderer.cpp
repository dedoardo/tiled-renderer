// Header
#include <camy_render/renderer.hpp>

// camy
#include <camy/gpu_backend.hpp>
#include <camy_render/camera.hpp>
#include <camy_render/scene.hpp>

// C++ STL
#include <algorithm>

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

	bool Renderer::load(Surface* window_surface, const float4x4& projection, u32 max_lights, u32 shadow_map_size)
	{
		unload();

		if (window_surface == nullptr)
		{
			camy_error("Tried to load renderer with null window_surface");
			return false;
		}

		m_window_surface = window_surface;
		m_max_lights = max_lights;

		m_offscreen_target = hidden::gpu.create_render_target(Surface::Format::RGBA16Float, window_surface->description.width, window_surface->description.height);
		if (m_offscreen_target == nullptr)
		{
			unload();
			return false;
		}

		if (!m_sky_pass.load(m_offscreen_target))
		{
			camy_error("Failed to create sky pass");
			unload();
			return false;
		}

		// Depth for the scene is the same resolution as backbuffer's one
		if (!m_scene_depth_pass.load(window_surface->description.width, window_surface->description.height, true))
		{
			camy_error("Failed to create scene depth pass");
			unload();
			return false;
		}

		if (!m_light_culling_pass.load(window_surface->description.width, window_surface->description.height))
		{
			camy_error("Failed to load light culling pass");
			unload();
			return false;
		}

		// Resolution for depth is different tho
		if (!m_light_depth_pass.load(shadow_map_size, shadow_map_size, true))
		{
			camy_error("Failed to create light depth pass");
			unload();
			return false;
		}

		if (!m_forward_pass.load(m_offscreen_target, max_lights))
		{
			camy_error("Failed to create forward pass");
			unload();
			return false;
		}

		// Setting dependencies
		//m_light_depth_out.type = Dependency::Type::DepthBuffer;
		//m_light_depth_out.resource = 0; // Not even used, there is only one depth buffer

		m_forward_deps[0] = m_forward_pass.get_shadow_map_var();

		m_forward_deps[1] = m_forward_pass.get_light_indices_var();

		m_forward_deps[2] = m_forward_pass.get_light_grid_var();

		m_forward_deps[3] = m_forward_pass.get_shadow_map_view_var();

		// Todo: Set scnee depth pass dependency

		if (!m_post_process_pipeline.load(m_offscreen_target, m_window_surface))
		{
			unload();
			return false;
		}

		m_pp_layer.create(&m_post_process_pipeline.pp_items[0], m_post_process_pipeline.pp_items.size());

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
		m_scene_depth_layer.begin(0);
		m_light_depth_layer.begin(0);
		m_forward_layer.begin(0);
		m_forward_layer.begin(1);

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
		m_sky_layer.begin(0);
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