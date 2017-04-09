/* render_context.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/math/types.hpp>
#include <camy/graphics/base.hpp>
#include <camy/graphics/resource_manager.hpp>

namespace camy
{
	class CommandList;

	/*
		Class: RenderContext
			Raw GPU Interface that allows for resource creation and commands list dispatchs.
			It can be used (encouraged )from multiple threads provided the costraints in <Usage>.
			It embeds a default NativeResourceStorage that can be freely used
			by the user.

			Note: The user should not use the RenderContext directly, except if 
			he wants to manually handle resource creation.

			release & aquire are backend specific ( They don't do almost anything on 
			D3D11 while on Opengl They unbind/bind different contexts to different
			threads ).
		Usage:
			
	*/
	class camy_api RenderContext final
	{
	public:
		RenderContext();
		~RenderContext();
		
		bool init(const StartupInfo& info_in, RuntimeInfo& info_out);
		void destroy();

		// Allows for multiple threads to acquire contexts for rendering / 
		// resource creation ( Have to define this correctly for OpenGL ) 
		// and command list creation
		// ContextID is a number between [0, kMaxConcurrentContexts)
		// The release/acquire interface is atomic but you shouldn't really
		// mess with it, simply acquire at the beginning and it's done. 
		void release(ContextID ctx_id);
		bool aquire(ContextID ctx_id);
	
		// Returns the ContextID ( if any ) associated with the current thread
		// not super expensive but cache it for the operation's scope.
		// used to access context-relevant native data.
		ContextID get_id_for_current();

		// Here simply to avoid friend classes, you should never touch it, 
		// Except if you need to interface with something that requires 
		// D3D/OGL data
		RenderContextData& get_platform_data();
		
		// Rendering
		/*
			Flushing is **not** thread-safe, this is due to the limitations 
			of D3D11/OGL4. There is no point in flushing from multiple threads,
			as it doesn't speed up execution the driver queue is unique.
		*/
		void flush(CommandList& command_list);

		void        immediate_clear(); // Resets the state of the immediate context (D3D11)
		void		clear_color(HResource target, const float4& color);  // helper (use commandlist)
		void		clear_depth(HResource target, float depth, sint32 stencil = -1);  // helper (use commandlist)
		void		immediate_cbuffer_update(HResource handle, void* data); 
		HResource   get_backbuffer_handle()const;
		Surface&    get_backbuffer();
		void        swap_buffers();

		HResource create_surface(const SurfaceDesc& desc, const SubSurface* subsurfaces = nullptr, rsize num_subsurfaces = 0, const char8* name = nullptr);
		HResource create_buffer(const BufferDesc& desc, const void* data = nullptr, const char8* name = nullptr);
		HResource create_vertex_buffer(const VertexBufferDesc& desc, const void* data = nullptr, const char8* name = nullptr);
		HResource create_index_buffer(const IndexBufferDesc& desc, const void* data = nullptr, const char8* name = nullptr);
		HResource create_constant_buffer(const ConstantBufferDesc& desc, const char8* name = nullptr);
		HResource create_blend_state(const BlendStateDesc& desc, const char8* name = nullptr);
		HResource create_rasterizer_state(const RasterizerStateDesc& desc, const char8* name = nullptr);
		HResource create_input_signature(const InputSignatureDesc& desc, const char8* name = nullptr);
		HResource create_sampler(const SamplerDesc& desc, const char8* name = nullptr);
		HResource create_depth_stencil_state(const DepthStencilStateDesc& desc, const char8* name = nullptr);
		HResource create_shader(const ShaderDesc& desc, const char8* name = nullptr);

		// Type not embedded in the handle. it has to be explicit
		void destroy_surface(HResource handle);
		void destroy_buffer(HResource handle);
		void destroy_vertex_buffer(HResource handle);
		void destroy_index_buffer(HResource handle);
		void destroy_constant_buffer(HResource handle);
		void destroy_blend_state(HResource handle);
		void destroy_rasterizer_state(HResource handle);
		void destroy_input_signature(HResource handle);
		void destroy_sampler(HResource handle);
		void destroy_depth_stencil_state(HResource handle);
		void destroy_shader(HResource handle);

		Surface&            get_surface(HResource handle);
		Buffer&             get_buffer(HResource handle);
		VertexBuffer&       get_vertex_buffer(HResource handle);
		IndexBuffer&        get_index_buffer(HResource handle);
		ConstantBuffer&     get_constant_buffer(HResource handle);
		BlendState&         get_blend_state(HResource handle);
		RasterizerState&    get_rasterizer_state(HResource handle);
		InputSignature&     get_input_signature(HResource handle);
		Sampler&            get_sampler(HResource handle);
		DepthStencilState&  get_depth_stencil_state(HResource handle);
		Shader&             get_shader(HResource handle);

	private:
		RenderContextData m_data;
		ResourceManager   m_resource_manager;
		HResource         m_backbuffer_handle;
	};
}