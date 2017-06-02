/* render_context.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/camy.hpp>
#include <camy/containers/pool.hpp>
#include <camy/graphics.hpp>

namespace camy
{
    class CommandList;

#pragma pack(push, 1)
    //! Handle to a resource, wrapped in a struct w/ cast and non-explicit constructur
    //! in order to provide a similar interface to ShaderVariable.
    struct CAMY_API HResource
    {
        uint16 _v = (uint16)-1;

        HResource(uint16 val = -1) { _v = val; }
        operator uint16() { return _v; }
        bool is_valid() const { return _v != (uint16)-1; }
        bool is_invalid() const { return _v == (uint16)-1; }

        constexpr static uint16 make_invalid() { return (uint16)-1; }
    };
#pragma pack(pop)

    class CAMY_API ResourceManager final
    {
      public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        template <typename ResourceType>
        HResource allocate();

        template <typename ResourceType>
        void deallocate(HResource handle);

        template <typename ResourceType>
        ResourceType& get(HResource handle);

      private:
        Pool<Surface> m_surfaces;
        Pool<Buffer> m_buffers;
        Pool<VertexBuffer> m_vertex_buffers;
        Pool<IndexBuffer> m_index_buffers;
        Pool<ConstantBuffer> m_constant_buffers;
        Pool<BlendState> m_blend_states;
        Pool<RasterizerState> m_rasterizer_states;
        Pool<Shader> m_shaders;
        Pool<InputSignature> m_input_signatures;
        Pool<Sampler> m_samplers;
        Pool<DepthStencilState> m_depth_stencil_states;
    };

    /*
            Class: RenderContext
                    Raw GPU Interface that allows for resource creation and commands list dispatchs.
                    It can be used (encouraged )from multiple threads provided the costraints in
       <Usage>.
                    It embeds a default NativeResourceStorage that can be freely used
                    by the user.

                    Note: The user should not use the RenderContext directly, except if
                    he wants to manually handle resource creation.

                    release & aquire are backend specific ( They don't do almost anything on
                    D3D11 while on Opengl They unbind/bind different contexts to different
                    threads ).
            Usage:

    */
    class CAMY_API RenderContext final
    {
      public:
        RenderContext();
        ~RenderContext();

        bool init(const StartupInfo& info_in, RuntimeInfo& info_out);
        void destroy();

        ContextID id_for_current();

        // Locks the specified context to the current thread and allows for operations
        bool acquire(ContextID ctx_id);

        // Releases a previously acquired context id
        void release(ContextID ctx_id);

        // Internal
        RenderContextData& get_platform_data();

        // Rendering
        /*
                Flushing is **not** thread-safe, this is due to the limitations
                of D3D11/OGL4. There is no point in flushing from multiple threads,
                as it doesn't speed up execution the driver queue is unique.
        */
        void flush(CommandList& command_list);

        HResource get_backbuffer_handle() const;
        Surface& get_backbuffer();
        void swap_buffers();

        HResource create_surface(const SurfaceDesc& desc,
                                 const SubSurface* subsurfaces = nullptr,
                                 rsize num_subsurfaces = 0,
                                 const char8* name = nullptr);
        HResource create_buffer(const BufferDesc& desc,
                                const void* data = nullptr,
                                const char8* name = nullptr);
        HResource create_vertex_buffer(const VertexBufferDesc& desc,
                                       const void* data = nullptr,
                                       const char8* name = nullptr);
        HResource create_index_buffer(const IndexBufferDesc& desc,
                                      const void* data = nullptr,
                                      const char8* name = nullptr);
        HResource create_constant_buffer(const ConstantBufferDesc& desc,
                                         const char8* name = nullptr);
        HResource create_blend_state(const BlendStateDesc& desc, const char8* name = nullptr);
        HResource create_rasterizer_state(const RasterizerStateDesc& desc,
                                          const char8* name = nullptr);
        HResource create_input_signature(InputSignatureDesc& desc, const char8* name = nullptr);
        HResource create_sampler(const SamplerDesc& desc, const char8* name = nullptr);
        HResource create_depth_stencil_state(const DepthStencilStateDesc& desc,
                                             const char8* name = nullptr);
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

        Surface& get_surface(HResource handle);
        Buffer& get_buffer(HResource handle);
        VertexBuffer& get_vertex_buffer(HResource handle);
        IndexBuffer& get_index_buffer(HResource handle);
        ConstantBuffer& get_constant_buffer(HResource handle);
        BlendState& get_blend_state(HResource handle);
        RasterizerState& get_rasterizer_state(HResource handle);
        InputSignature& get_input_signature(HResource handle);
        Sampler& get_sampler(HResource handle);
        DepthStencilState& get_depth_stencil_state(HResource handle);
        Shader& get_shader(HResource handle);

      private:
        RenderContextData m_data;
        ResourceManager m_resource_manager;
        HResource m_backbuffer_handle;
        ContextID m_render_ctx;
    };
}

#include "render_context.inl"