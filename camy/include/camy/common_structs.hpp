#pragma once

// camy
#include "base.hpp"
#include "resources.hpp"
#include "features.hpp"
#include "shader.hpp"
#include "math.hpp"

namespace camy
{
	struct DrawInfo
	{
		u32 vertex_offset{ 0 };
		u32 index_offset{ 0 };
		u32 index_count{ 0 };
		PrimitiveTopology primitive_topology;
	};

	struct CommonStates
	{
		Surface* render_targets[features::max_cachable_rts]{ nullptr, nullptr };
		Surface* depth_buffer{ nullptr };

		RasterizerState* rasterizer_state{ nullptr };
		BlendState*		 blend_state{ nullptr };
		DepthStencilState* depth_stencil_state{ nullptr };
		
		// No default exists yet for viewport, has to be set manually
		Viewport viewport{ 0.f, 0.f, 0.f, 0.f };
	};

	/*
		camy is stateless, but this doesn't really apply to resources bound to the pipeline, 
		this can result into some leaking of resources from one renderqueue to the other. 
		This usually causes hazards and in order to avoid this we have dependencies, a renderqueue has a dependency list attached to it, meaning that at the end of the frame the dependencies:
		`using Dependency = ShaderVariable`
		are "manually" unbound from the pipeline, this is especially useful in render to texture scenarios. Note that care must be taken also for dependencies inbetween frames, there is no reset for resources.
	*/
	using Dependency = ShaderVariable;

	struct PipelineParameter
	{
		ShaderVariable    shader_variable{ 1u };
		const void* data{ nullptr }; // usually either a PipelineResource or raw data
	};

	/*
		A parameter group groups all the resources that will be bound to the pipeline
		for a specific drawcall. They are all arrays for performance reasons and simplicity
		and are usually created from a pool
	*/
	struct ParameterGroup
	{
		PipelineParameter* parameters{ nullptr };
		u32				   num_parameters{ 0u };
	};

	/*
		Without tying the renderer and the backend too much there is no straighforward way to
		cache all the possible shader resource views & samplers, yes it could be done keeping
		an array of 128 * shader stages resource views + sampler, but it's a very ugly solution. 
		Queueing up items in the render layer state caching cannot be done since they will 
		be sorted afterwards.

		RenderItem still need some per "render item" properties, such as textures, matrices and 
		material data. Each set of parameters ( can be more than one as many as you like ) is 
		cached together and has an associated cache slot ( 0 <= cache_slot <= features::num_cache_slots ) )


		Example: 
			Material parameter, you create e ParameterGroup that has a series of parameter
				
	*/
	struct CachedParameterGroup
	{
		u32				 cache_slot;
		const ParameterGroup*  parameter_group;
	};

	/*
		Currently a render item holds everything, to minimize memory usage, in the 
		future we might parameter groups or state groups and only include things that 
		are used or changed from the previous render item.
		We could have a RenderItem at the beginning whose sole purpose would be to set
		the shared state and the subsequence just include used states

		http://www.gamedev.net/topic/636389-advanced-render-queue-api/
		http://gamedev.stackexchange.com/questions/50249/managing-graphic-state-and-components
	*/
	struct RenderItem
	{		
		using Key = u64;

		Key key{ 0 };

		DrawInfo draw_info;
	
		VertexBuffer* vertex_buffer1{ nullptr };
		VertexBuffer* vertex_buffer2{ nullptr };
		IndexBuffer*  index_buffer{ nullptr };

		Shader*	vertex_shader{ nullptr };
		Shader*	pixel_shader{ nullptr };
		Shader*	geometry_shader{ nullptr };

		CommonStates* common_states{ nullptr };

		CachedParameterGroup cached_parameter_groups[features::num_cache_slots];
		u32					 num_cached_parameter_groups{ 0 };
	};

	struct ComputeItem
	{
		using Key = u32;

		Key key{ 0 };
		u32 group_countx{ 0 };
		u32 group_county{ 0 };
		u32 group_countz{ 0 };

		Shader* compute_shader{ nullptr };
		ParameterGroup parameters;
	};

	/*
		Postprocessing: 
			A postprocessing step is a full screen effect that has the following characteristics:
			- No vertex shader / vertex buffers / index buffers( provided by default )
			- Pixel shader
			- Shared parameter group, as with the other passes they are not changed between passes and are always bound, they usually include environment variables ( screen, constants ) 
			- Input render target, if set to nullptr is the result of the previous pass, otherwise is the one specified, note that it is ALWAYS bound as shader resource slot 0, it's reserved by default
		When writing pixel shaders for postprocessing effects include pp_common and use the: camy_pp_input_surface, that simply defines a Texture2D bound as shader resource at slot0
	*/
	struct PostProcessItem
	{
		Surface*		input_surface{ nullptr };	// As said it's bound by default at slot 0, if it's nullptr the output_surface of the previous postprocessitem iwll be used
		ParameterGroup	parameters;					// Parameters for this specific pass, they are not probably needed, if they are shared with other passes, use the default ones, they are not cached
		Shader*			pixel_shader{ nullptr };	// Pixel shader that will be run on every texel of the output surface
		Surface*		output_surface{ nullptr };  // Output surface :) 
	
		BlendState*		blend_state{ nullptr };
	};
} 