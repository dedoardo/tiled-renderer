#pragma once

// camy
#include <camy/common_structs.hpp>
#include <camy/allocators/paged_linear_allocator.hpp>

// render
#include "shader_common.hpp"
#include "scene_node.hpp"

/*
	Topic: passes.hpp
		Here all the data structures needed for the passes are laid out, they 
		are then used by the renderer: 

	Topic: pipeline:
		Depth-only pass with camera view => Light frustums
		Depth-only pass with light view  => Shadow map
		Light-culling pass				 => Culls light, => light buffer/grid
		Sky pass						 => Rendering sky on final framebuffer
		Forward	pass					 => Rendering scene on final framebuffer
*/

namespace camy
{
	// Forward declaration
	class GPUBackend;
	
	/*
		Class: DepthPass 
			Helps rendering the scene outputting view-space depth as render target and 0-1 non-linear ( default ) depth
			as depth buffer, both can be retrieved. Output to the render target can be disabled, entirely skipping actual
			the pixel shader
	*/
	class DepthPass final
	{
	public:
		DepthPass();
		~DepthPass();

		/*
			Function: load 
				Prepares all the required pipeline resources, if output_view_as_rt is set to true then a render target will be bound and the pixel
				shader will be run
		*/
		bool load(u32 target_width, u32 target_height, bool output_view_as_rt);
	
		/*
			Function: unload
				Releases all the resources, it's done automatically in the destructor anyway
		*/
		void unload();

		/*
			Function: pre
				Called before the queueing phase begins, view and projection are separate because if outputting view as render target we just need the view matrix
		*/
		void pre(const float4x4& view, const float4x4& projection);
		
		/*
			Function: prepare
				Prepares the renderitem for rendering setting all the required states / parameters
		*/
		camy_inline void prepare(const RenderSceneNode* render_node, u32 renderable_index, RenderItem& render_item_out);

		/*
			Function: post
				Called after the queuing phase has eneded, but before the actual end() is called on the command/compute layer, here all the resources ( if needed are finalized )
		*/
		void post();

	public:
		const Surface* get_depth_buffer()const;
		const Surface* get_render_target()const;
		bool is_output_view_as_rt_enabled()const;
	
		const ParameterGroup* get_shared_parameters()const;

	private:
		CommonStates m_common_states;
 
		// Either one of the two depending whether view output is enabled
		shaders::PerFrame m_per_frame_data;
		shaders::PerFrameView m_per_frame_view_data;

		// Shared by all renderables
		PipelineParameter m_data_parameter;
		ParameterGroup m_parameter_group;

		// Used to allocate parameters/ groups for render items
		allocators::PagedLinearAllocator<sizeof(ParameterGroup) * 100> m_parameter_group_allocator;
		allocators::PagedLinearAllocator<sizeof(PipelineParameter) * 100>  m_data_parameter_allocator;
	
		bool   m_output_view_as_rt;
		Shader m_vertex_shader;
		Shader m_pixel_shader;
		
		// Here just to avoid calling a get() each time
		ShaderVariable m_per_object_var;
	};

	/*
		Class: LightCullingPass
			Single compute item that culls light in the scene and outputs two buffers:
				Light grid
				Light indices
			For more info :  http://www.slideshare.net/DICEStudio/directx-11-rendering-in-battlefield-3
	*/
	class LightCullingPass final
	{
	public:
		LightCullingPass();
		~LightCullingPass();

		/*
			Function: load
		*/
		bool load(u32 target_width, u32 target_height);
		void unload();

		/*
			Function: single_prepare
				no pre/post because one single compute item is to be used
		*/
		camy_inline void prepare_single(const Buffer* lights_buffer, const Surface* view_rt, const float4x4& view, const float4x4& projection, u32 num_lights, ComputeItem& compute_item_out);

	public:
		const Buffer* get_light_indices()const;
		ShaderVariable get_light_indices_var()const;

		const Buffer* get_light_grid()const;
		ShaderVariable get_light_grid_var()const;

	private:
		Shader m_compute_shader;

		shaders::CullingDispatchArgs m_culling_dispatch_args;

		ParameterGroup		m_parameter_group;
		PipelineParameter   m_parameters[6]; // Surface, data, 4 buffers

		Buffer* m_next_light_index;
		Buffer* m_light_indices;
		Buffer* m_light_grid;
	};
	
	/*
		Class: 
			Single item that renders the skybox
	*/
	class SkyPass final
	{
	public:
		SkyPass();
		~SkyPass();

		bool load(Surface* target_surface);
		void unload();

		camy_inline void prepare_single(const Camera& camera, RenderItem& render_item_out);
		
	public:
		const ParameterGroup* get_shared_parameter_group()const;

	private:
		CommonStates   m_common_states;

		Shader m_vertex_shader;
		Shader m_pixel_shader;

		VertexBuffer* m_vertex_buffer;
		IndexBuffer*  m_index_buffer;

		shaders::PerFrameAndObject m_per_frame_object;
		PipelineParameter  m_data_parameter;
		ParameterGroup m_parameter_group;
	};

	class ForwardPass
	{
	public:
		ForwardPass();
		~ForwardPass();

		bool load(Surface* target_surface, const u32 max_lights);
		void unload();

		void pre(const Camera& camera, const float4x4& light_view, const float4x4& light_projection, const Surface* shadow_map, const Surface* shadow_map_view, const Buffer* light_indices, const Buffer* light_grid);
		camy_inline void prepare(const RenderSceneNode* render_node, u32 renderable_index, RenderItem& render_item_out);
		camy_inline void add_light(const LightSceneNode* node);
		void post(const Buffer* light_indices, const Buffer* light_grid);

	public:
		const ParameterGroup* get_shared_parameters()const;

		const Buffer* get_light_buffer()const;
		ShaderVariable get_shadow_map_var()const;
		ShaderVariable get_shadow_map_view_var()const;

		ShaderVariable get_light_indices_var()const;
		ShaderVariable get_light_grid_var()const;

		u32 get_num_lights()const;

	private:
		CommonStates m_common_states;

		shaders::PerFrameLight m_per_frame;
		shaders::Environment   m_environment;
		ParameterGroup		   m_parameter_group;
		PipelineParameter	   m_parameters[2 + 2 + 1 + 3]; // sampler, data, surface, buffers

		allocators::PagedLinearAllocator<sizeof(ParameterGroup) * 100> m_parameter_group_allocator;
		allocators::PagedLinearAllocator<sizeof(PipelineParameter) * 100>  m_parameters_allocator;

		Shader m_vertex_shader;
		Shader m_pixel_shader;

		u32 m_max_lights;
		u32 m_next_light;
		shaders::Light* m_light_data;
		Buffer*			m_light_buffer;
	};
}

#include "passes.inl"