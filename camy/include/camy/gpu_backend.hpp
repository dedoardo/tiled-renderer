#pragma once

// camy
#include "base.hpp"
#include "resource_storer.hpp"
#include "shader.hpp"
#include "common_structs.hpp"
#include "cbuffer_system.hpp"

namespace camy
{
	// Forward declaration
	class RenderLayer;
	class ComputeLayer;
	class PostProcessLayer;
	struct PipelineCache;

	class GPUBackend final
	{
	public:
		GPUBackend();
		~GPUBackend();

		bool open(u32 adapter_index = 0);
		void close();

		// No purpose in moving / copying, since this is the "core" of the whole library
		// It might have implications that have yet to be considered
		GPUBackend(GPUBackend& other) = delete;
		GPUBackend& operator=(GPUBackend& other) = delete;
		GPUBackend(GPUBackend&& other) = delete;
		GPUBackend& operator=(GPUBackend&& other) = delete;

		/*
			We could have had one more layer of abstraction where RenderItems are translated into commands as suggested by
			hodgman http://www.gamedev.net/topic/636389-advanced-render-queue-api/
			but to be honest there is no need for such a thing here *yet* might implement it later when supporting another backend,
			the idea would be to execute on an already compiled command list instead of on a bunch of render layers
		*/
		void execute(const RenderLayer* render_layer);

		void execute(const ComputeItem& compute_item);

		void execute(const ComputeLayer* compute_layer);

		void execute(const PostProcessLayer* pp_layer);

		void update(const Buffer* buffer, const void* data);

		// Black-box not intended to be used by the user, here just to mantain some encapsulation, the main reason is that inputs is dependant on the platform
		
		/*
			All create functions
		*/
		
		/*
			Function:
				Creates a new input signature, note that this if you are using the Shader class the input signature is created automatically
				and this very function is more of a design-flaw. The inputs parameter is platform specific. 

			compiled_bytecode - Vertex shader compiled bytecode for validation
			bytecode_size	  - Size in bytes of the bytecode
			inputs			  - Platform specific array of elements to create the input layout with ( D3D11_INPUT_ELEMENT_DESC )
			num_imputs		  - Number of elements in the inputs array
		*/
		InputSignature* create_input_signature(const void* compiled_bytecode, Size bytecode_size, const void* inputs, Size num_inputs);

		/*
			Function: create_buffer
				Creates a new buffer, can be of any of the types Buffer::Type for Vertex, Index and Constant buffers see respective create_**** functions,
				buffers are bound as shader resources with samplers/textures etc...
			
			type - Type of the buffer
			element_count - number of elements in the buffer
			element_size - size of each element
			For how to use the parameters see Buffer::Type
		*/
		Buffer* create_buffer(Buffer::Type type, u32 num_elements, u32 element_size, bool use_uav = false);

		/*
			Function: create_vertex_buffer
				Creates a vertex buffer
			
			element_size	- size of each element
			element_count	- number of elements in the buffer
			data			- pointer to buffer of size element_size * element_count for initial data
			is_dynamic		- true if you want the resource to be dynamic and easily updateable every frame
		*/
		VertexBuffer* create_vertex_buffer(u32 element_size, u32 num_elements, const void* data = nullptr, bool is_dynamic = false);
		
		/*
			Function: create_index_buffer
				Creates an index buffer

			index_type		- specifies what type of index to use ( 16 or 32 bits)
			element_count	- number of indices in the buffer
			data			- data to initialize the buffer with
			is_dynamic		- true if you want the resource to be dynamic 
		*/
		IndexBuffer* create_index_buffer(IndexBuffer::Type index_type, u32 num_elements, const void* data = nullptr, bool is_dynamic = false);
		
		/*
			Function: create_constant_buffer
				Use Shader <shader.hpp>
		*/
		ConstantBuffer* create_constant_buffer(u32 size);
		
		/*
			Function: create_texture2D
				Creates a surface used exclusively as input for a shader
			
			format - format used by the surface
			width  - width in pixels
			height - height in pixels
			pitch  - If you are providing some initial data, this has to be the size in bytes of a row
			data   - Initial data, size must be pitch * height
			is_dynamic - true if you want the resource to be dynamic
			msaa_level - is it an msaa_level, it has to be > 0
		*/
		Surface* create_texture2D(Surface::Format format, u32 width, u32 height, const SubSurface* subsurfaces = nullptr, u32 num_subsurfaces = 0, bool is_dynamic = false, u8 msaa_level = 1);
		
		/*
			Function: create_render_target
				Creates a surface usable as render target and shader input

			format - depth/stencil format used by the surface
			width  - width in pixels
			height - height in pixels
			msaa_level - is it an msaa_level, it has to be > 0
		*/
		Surface* create_render_target(Surface::Format format, u32 width, u32 height, u8 msaa_level = 1);
		

		/*
			Function: create_depth_buffer
				Creates a depth ( and possibly stencil ) buffer ( depending on the format ). Note that it is usable exclusively as 
				depth view, nothing else!
			format - depth/stencil format used by the surface
			width  - width in pixels
			height - height in pixels
			msaa_level - is it an msaa_level, it has to be > 0
		*/
		Surface* create_depth_buffer(Surface::Format format, u32 width, u32 height, u8 msaa_level = 1);
		
		/*
			Function: create_surface
				Creates a surface that can be interpreter differently at different stages, all the pixel sizes have to match,
				this is especially useful for e.g. shadow maps

			description - full description of the surface
			use_srv		- true if it will be serve as input for a hsader
			use_rtv		- true if it will be used as render target
			use_dsv		- true if it will be used as depth buffer
		*/
		Surface* create_surface(const Surface::Description& description, bool use_srv, bool use_rtv, bool use_dsv, bool use_uav = false, const SubSurface* subsurfaces = nullptr, u32 num_subsurfaces = 0);
		
		/*
			Function: create_blend_state
				Creates a blend state from one of the specific presets ( nothing custom is yet supported

			blend_mode - Preset
		*/
		BlendState* create_blend_state(BlendState::Mode blend_mode);

		/*
			Function: create_rasterizer_state
				Creates a rasterizer state 

			see D3D11_RASTERIZER_DESC
		*/
		RasterizerState* create_rasterizer_state(RasterizerState::Cull cull, RasterizerState::Fill fill, u32 bias = 0, float bias_max = 0.f, float bias_slope = 0.f);
		
		/*
			Function: create_sampler
				Creates a sampler  ( N
		*/
		Sampler* create_sampler(Sampler::Filter filter, Sampler::Address address, Sampler::Comparison comparison = Sampler::Comparison::Never);
		
		/*
			Function: create_shader
				use Shader <shader.hpp>
		*/
		hidden::Shader* create_shader(Shader::Type type, const void* compiled_bytecode, Size bytecode_size);
		
		DepthStencilState* create_depth_stencil_state();

		/*
			Function: create_window_surface
				creates a surface usable as render target and shader resource, format is R8G8B8A8 Unorm
		*/
		Surface* create_window_surface(WindowHandle window_handle, u8 msaa_level = 1);

		template <typename Type>
		void dispose(Type* ptr);

		template <typename Type>
		void safe_dispose(Type*& ptr);

		void clear_surface(const Surface* surface, const float* color, const float depth, const u32 stencil);

		void swap_buffers(const Surface* window_surface);

	private:
		bool create_builtin_resources();

		// Functions are here merely for clarity in the execute(***) code
		camy_inline void set_parameters(const ParameterGroup& parameters, PipelineCache& pipeline_cache);
		camy_inline void set_parameter(const PipelineParameter& parameter, PipelineCache& pipeline_cache);
		camy_inline void set_common_states(const CommonStates& common_states);
		camy_inline void set_default_common_states();
		camy_inline void unbind_dependency(const Dependency& dependency);

	private:
		ResourceStorer m_resources;
		
		CBufferSystem m_cbuffers[Shader::num_types];
		CommonStates  m_default_states;

		ID3D11Device*			m_device;
		ID3D11DeviceContext*	m_context;
		IDXGIFactory*			m_factory;
		IDXGIAdapter*			m_adapter;
		GraphicsAPIVersion		m_feature_level;
		
		// Built-in resources ( Todo: one all effects are implemented might move them somewhere else )
		hidden::Shader*	m_postprocess_vs;
	};
}

#include "gpu_backend.inl"