#pragma once

// camy
#include <camy/layers.hpp>
#include <camy/layer_dispatcher.hpp>

// render
#include "passes.hpp"
#include "post_process_pipeline.hpp"

// C++ STL
#include <vector>

namespace camy
{
	class GPUBackend;
	class Scene;
	class Camera;
	struct Viewport;

	/*
		Class: Renderer
			Actual class that is in duty of rendering, does nothing else, he receives an already processed
			scene an renders, for more information on the technique read the Documentation
	*/
	class Renderer final
	{
	public:
		Renderer();
		~Renderer();

		Renderer(const Renderer& other) = delete;
		Renderer& operator=(const Renderer& other) = delete;

		Renderer(Renderer&& other) = delete;
		Renderer& operator=(Renderer&& other) = delete;

		/*
			Function: load
				Creates all the required resources needed for rendering based on the
				effects that are being enabled. 
		*/
		bool load(Surface* window_surface, const float4x4& projection, u32 effects, u32 max_lights = 10, u32 shadow_map_size = 4096);
		void unload();

		// Todo: provide default viewport value
		void render(Scene& scene, Camera& camera); // Default viewport that corresponds to the window_surface specified in load();
		void render(Scene& scene, Camera& camera, const Viewport& viewport);

		void sync();

	private:		
		void _queue_sky(Scene& scene, Camera& camera, const Viewport& viewport);
		void _queue_light_culling(Scene& scene, Camera& camera, const Viewport& viewport);

		u32		 m_effects;
		Surface* m_window_surface;
		Surface* m_offscreen_target;
		PostProcessPipeline m_post_process_pipeline;
		
		LayerDispatcher m_layer_dispatcher;
		ComputeLayer m_light_culling_layer;
		RenderLayer m_sky_layer;
		RenderLayer m_scene_depth_layer;
		RenderLayer m_light_depth_layer;
		RenderLayer m_forward_layer;
		PostProcessLayer m_pp_layer;

		SkyPass m_sky_pass;
		DepthPass m_scene_depth_pass;
		DepthPass m_light_depth_pass;
		LightCullingPass m_light_culling_pass;
		ForwardPass m_forward_pass;

		Dependency m_scene_depht_out;
		Dependency m_light_depth_out;
		Dependency m_forward_deps[4];

		u32 m_max_lights;
	};
}