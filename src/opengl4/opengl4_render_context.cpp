/* render_context.cpp
*
*Copyright (C) 2017 Edoardo Dominici
*
*This software may be modified and distributed under the terms
*of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/render_context.hpp>

#if defined(CAMY_OS_WINDOWS) && defined(CAMY_BACKEND_OPENGL4)

// camy
#include <camy/command_list.hpp>
#include <camy/opengl4/opengl4_command_list.hpp>
#include <camy/system.hpp>

#define CAMY_CHECK_CONTEXT_FOR_THREAD                                                              \
    ContextID ctx_id = id_for_current();                                                           \
    if (ctx_id == ::camy::API::INVALID_CONTEXT_ID)                                                 \
    {                                                                                              \
        CL_ERR("Failed to use RenderContext as the current thread hasn't acquired one");           \
        return HResource::make_invalid();                                                          \
    }

namespace camy
{
    using namespace OpenGL4;

    namespace
    {
        int camy_to_bytesize(InputElement::Type type)
        {
            switch (type)
            {
            case InputElement::Type::Float3:
                return sizeof(float3);
            case InputElement::Type::Float4:
                return sizeof(float4);
            default:
                return 0;
            }
        }

        int camy_to_opengl_comps(InputElement::Type type)
        {
            switch (type)
            {
            case InputElement::Type::Float3:
                return 3;
            case InputElement::Type::Float4:
                return 4;
            default:
                return -1;
            }
        }

        int camy_to_opengl_type(InputElement::Type type)
        {
            switch (type)
            {
            case InputElement::Type::Float3:
            case InputElement::Type::Float4:
                return GL_FLOAT;
            default:
                return GL_NONE;
            }
        }
    }

    bool init_input_signature(InputSignature& is)
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);

        // http://stackoverflow.com/questions/21153729/glbindvertexbuffer-vs-glbindbuffer
        // TODO: Support multiple bindings
        int offset = 0;
        for (rsize i = 0; i < is.desc.num_elements; ++i)
        {
            InputElement const& element = is.desc.elements[i];
            glVertexArrayVertexAttribFormatEXT(vao, element.slot,
                                               camy_to_opengl_comps(element.type),
                                               camy_to_opengl_type(element.type), GL_FALSE, offset);
            offset += camy_to_bytesize(element.type);
        }

        if (OpenGL4::has_errors())
        {
            CL_ERR("Error creating input signature, see above for more details");
            return false;
        }

        is.native.vao = vao;
        return true;
    }

    extern HWND create_window(const StartupInfo& info, bool is_registered = false);
    extern void destroy_window(HWND hwnd);

    RenderContext::RenderContext() {}

    RenderContext::~RenderContext() { destroy(); }

    bool RenderContext::init(const StartupInfo& info_in, RuntimeInfo& info_out)
    {
        destroy();

        HWND dummy_hwnd = create_window(info_in);

        if (!dummy_hwnd)
        {
            CL_ERR("Failed to create dummy window");
            return false;
        }

        CL_INFO("Creating RenderContext..");
        CL_INFO("Available contexts: ", API::MAX_CONTEXTS);

        HDC dummy_hdc = GetDC(dummy_hwnd);

        // Dummy pixelformat
        PIXELFORMATDESCRIPTOR dummy_pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            32,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            24,
            8,
            0,
            PFD_MAIN_PLANE,
            0,
            0,
            0,
            0};

        int dummy_pf = ChoosePixelFormat(dummy_hdc, &dummy_pfd);
        if (dummy_pf == 0)
        {
            CL_ERR("Failed to find matching pixel format");
            return false;
        }
        SetPixelFormat(dummy_hdc, dummy_pf, &dummy_pfd);
        HGLRC dummy_hrc = wglCreateContext(dummy_hdc);
        wglMakeCurrent(dummy_hdc, dummy_hrc);

        // Loading extensions
        glewExperimental = true;
        if (glewInit() != GLEW_OK)
        {
            CL_ERR("Failed to initialize glew");
            ReleaseDC(dummy_hwnd, dummy_hdc);
            return false;
        }

        // Destroying dummy context
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(dummy_hrc);
        ReleaseDC(dummy_hwnd, dummy_hdc);
        destroy_window(dummy_hwnd);

        // Create new window
        HWND hwnd = create_window(info_in, true);
        if (!hwnd)
        {
            CL_ERR("Failed to create window");
            return false;
        }

        HDC hdc = GetDC(hwnd);

        const int attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB,
            GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,
            GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,
            GL_TRUE,
            WGL_PIXEL_TYPE_ARB,
            WGL_TYPE_RGBA_ARB,
            WGL_SAMPLE_BUFFERS_ARB,
            GL_TRUE,
            WGL_SAMPLES_ARB,
            info_in.msaa,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,
            info_in.is_srgb,
            WGL_COLOR_BITS_ARB,
            32,
            WGL_DEPTH_BITS_ARB,
            24,
            WGL_STENCIL_BITS_ARB,
            8,
            0,
        };
        int pixel_format;
        UINT num_formats;
        wglChoosePixelFormatARB(hdc, attribs, nullptr, 1, &pixel_format, &num_formats);
        SetPixelFormat(hdc, pixel_format, &dummy_pfd);

        int flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#if defined(camy_gpu_context_debug)
        flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

        // Creating proper context
        int ctx_attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                             4,
                             WGL_CONTEXT_MINOR_VERSION_ARB,
                             5,
                             WGL_CONTEXT_FLAGS_ARB,
                             flags,
                             WGL_CONTEXT_PROFILE_MASK_ARB,
                             WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                             0};

        m_data.render_ctx = wglCreateContextAttribsARB(hdc, nullptr, ctx_attribs);
        m_data.hdc = hdc;

        if (!m_data.render_ctx)
        {
            CL_ERR("wglCreateContextAttribsARB Failed with error: ", GetLastError());
            ReleaseDC(hwnd, hdc);
            destroy_window(hwnd);
            return false;
        }

        Surface backbuffer;
        // TODO: Shouldnt it be GetClientRect ?
        backbuffer.desc.width = info_in.window.width;
        backbuffer.desc.height = info_in.window.height;
        backbuffer.desc.msaa_levels = info_in.msaa;
        backbuffer.native.texture = 0;
        backbuffer.native.iformat = GL_RGBA8;

        m_backbuffer_handle = m_resource_manager.allocate<Surface>();
        m_resource_manager.get<Surface>(m_backbuffer_handle) = backbuffer;

        for (rsize i = 0; i < API::MAX_CONTEXTS; ++i)
        {
            m_data.contexts[i].off_ctx = wglCreateContextAttribsARB(hdc, nullptr, ctx_attribs);
            m_data.contexts[i].locked = false;
            wglShareLists(m_data.render_ctx, m_data.contexts[i].off_ctx);
        }

        wglMakeCurrent(hdc, m_data.render_ctx);

        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        m_data.surface.desc.width = client_rect.right - client_rect.left;
        m_data.surface.desc.height = client_rect.bottom - client_rect.top;

        m_render_ctx = info_in.render_ctx;

        return true;
    }

    void RenderContext::destroy() {}

    bool RenderContext::acquire(ContextID ctx_id)
    {
        CAMY_ASSERT(m_data.is_valid());

        if (ctx_id >= API::MAX_CONTEXTS)
        {
            CL_ERR("Invalid argument: ctx_id allowed range is [0, ", API::MAX_CONTEXTS,
                   "(API::MAX_CONTEXTS)]");
            return false;
        }

        uint32 des = 1; // We want to lock it
        if (API::atomic_cas(m_data.contexts[ctx_id].locked, 0, des) == 0)
        {
            wglMakeCurrent(m_data.hdc, m_data.contexts[ctx_id].off_ctx);
            m_data.contexts[ctx_id].owner = API::thread_current();
            CL_INFO("Acquired context: ", ctx_id, " on thread: ", API::thread_current());
            return true;
        }

        CL_ERR("Failed to acquire context: ", ctx_id, " on thread: ", API::thread_current());
        return false;
    }

    void RenderContext::release(ContextID ctx_id)
    {
        CAMY_ASSERT(m_data.is_valid());

        if (ctx_id >= API::MAX_CONTEXTS)
        {
            CL_ERR("Invalid argument: ctx_id allowed range is [0, ", API::MAX_CONTEXTS,
                   "(API::MAX_CONTEXTS)]");
            return;
        }

        uint32 des = 0; // Unlocking
        if (API::atomic_cas(m_data.contexts[ctx_id].locked, 1, des) == 1)
            wglMakeCurrent(nullptr, nullptr);
        else
            CL_ERR("Failed to release context: ", ctx_id, " on thread: ", API::thread_current());
    }

    ContextID RenderContext::id_for_current()
    {
        CAMY_ASSERT(m_data.is_valid());
        ThreadID cur_id = API::thread_current();

        for (uint32 i = 0; i < API::MAX_CONTEXTS; ++i)
        {
            if (m_data.contexts[i].locked && m_data.contexts[i].owner == cur_id) return i;
        }

        return API::INVALID_CONTEXT_ID;
    }

    RenderContextData& RenderContext::get_platform_data() { return m_data; }

    void RenderContext::flush(CommandList& command_list)
    {
        CAMY_ASSERT(m_data.is_valid());

        ContextID ctx_id = id_for_current();
        if (ctx_id != m_render_ctx)
        {
            CL_ERR("Failed to flush command list, calling thread is not registered as render "
                   "context (StartupInfo)");
            return;
        }

        // Uploading ConstantBuffer data
        for (rsize i = 0; i < command_list.m_updates.count(); ++i)
        {
            CommandList::ConstantBufferUpdate& update = command_list.m_updates[i];
            ConstantBuffer& cbuffer = get_constant_buffer(update.handle);

            void* mapped_cbuffer = glMapNamedBuffer(cbuffer.native.buffer, GL_WRITE_ONLY);
            std::memcpy(mapped_cbuffer, update.data, update.bytes);
            glUnmapNamedBuffer(cbuffer.native.buffer);
        }

        // Time to create the container
        for (rsize i = 0; i < command_list.m_data.init_requests.count(); ++i)
        {
            InitResRequest& req = command_list.m_data.init_requests[i];
            if (req.type == InitResRequest::Type::Framebuffer)
            {
                GLuint fbo;
                glGenFramebuffers(1, &fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);

                for (rsize s = 0; s < API::MAX_RENDER_TARGETS; ++s)
                {
                    HResource handle = req.framebuffer.render_targets[s];
                    if (handle.is_invalid()) continue;
                    Surface& surface = get_surface(handle);
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + s,
                                         surface.native.texture, 0);
                }

                HResource handle = req.framebuffer.depth_buffer;
                if (handle.is_valid())
                {
                    Surface& surface = get_surface(handle);
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                         surface.native.texture, 0);
                }

                *command_list.m_data.fbo_map.find(req.hash) = (uint64&)fbo;
            }
            else if (req.type == InitResRequest::Type::ProgramPipeline)
            {
                GLuint ppo;
                glGenProgramPipelines(1, &ppo);

                HResource vs_handle = req.program_pipeline.vertex_shader;
                if (vs_handle.is_valid())
                {
                    Shader& shader = get_shader(vs_handle);
                    glUseProgramStages(ppo, GL_VERTEX_SHADER_BIT, shader.native.shader);
                }

                HResource ps_handle = req.program_pipeline.pixel_shader;
                if (ps_handle.is_valid())
                {
                    Shader& shader = get_shader(ps_handle);
                    glUseProgramStages(ppo, GL_FRAGMENT_SHADER_BIT, shader.native.shader);
                }

				*command_list.m_data.fbo_map.find(req.hash) = (uint64&)ppo;
            }
            else
                CAMY_ASSERT(false);
        }
        command_list.m_data.init_requests.clear();

        // Resetting state ?

        // Processing commands
        const byte* cur = command_list.m_data.command_buffer.data();
        const byte* end = cur + command_list.m_data.command_buffer.count();

        OpenGL4::flush_errors();
        while (cur < end)
        {
            uint16 op = Cmd::read<uint16>(cur);
            Cmd::cmd_ftbl[op](m_data, command_list.m_data, cur);
        }
    }

    HResource RenderContext::get_backbuffer_handle() const { return m_backbuffer_handle; }

    Surface& RenderContext::get_backbuffer() { return m_data.surface; }

    void RenderContext::swap_buffers() { SwapBuffers(m_data.hdc); }

    bool ogl_is_compressed(PixelFormat pformat)
    {
        switch (pformat)
        {
        case PixelFormat::BC1Unorm:
        case PixelFormat::BC5Unorm:
            return true;
        default:
            return false;
        }
    }

    void ogl_to_camy(PixelFormat pformat, GLenum& iformat, GLenum& format, GLenum& type)
    {
        switch (pformat)
        {
        case PixelFormat::Unknown:
            CAMY_ASSERT(false); // DXGI_FORMAT_UNKNOWN ?

        // Compressed formats
        case PixelFormat::BC1Unorm:
            iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case PixelFormat::BC3Unorm:
            iformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case PixelFormat::BC5Unorm:
            iformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

        // Typeless formats
        case PixelFormat::R8Typeless:
        case PixelFormat::R16Typeless:
        case PixelFormat::R32Typeless:
        case PixelFormat::R24G8Typeless:
        case PixelFormat::R24UnormX8Typeless:
            CAMY_ASSERT(false);

        // -> Single channel uncompressed
        case PixelFormat::R8Unorm:
            iformat = GL_R8;
            format = GL_RED;
            type = GL_UNSIGNED_BYTE;
            return;
        case PixelFormat::R16Unorm:
            iformat = GL_R16;
            format = GL_RED;
            type = GL_UNSIGNED_SHORT;
            return;
        case PixelFormat::R16Float:
            iformat = GL_R16F;
            format = GL_RED;
            type = GL_FLOAT;
            return;
        case PixelFormat::R32Float:
            iformat = GL_R32F;
            format = GL_RED;
            type = GL_FLOAT;
            return;

        // -> Two channels uncompressed
        case PixelFormat::RG8Unorm:
            iformat = GL_RG8;
            format = GL_RG;
            type = GL_UNSIGNED_BYTE;
            return;
        case PixelFormat::RG16Unorm:
            iformat = GL_RG16;
            format = GL_RG;
            type = GL_UNSIGNED_SHORT;
            return;
        case PixelFormat::RG16Float:
            iformat = GL_RG16F;
            format = GL_RG;
            type = GL_FLOAT;
            return;
        case PixelFormat::RG32Float:
            iformat = GL_RG32F;
            format = GL_RG;
            type = GL_FLOAT;
            return;

        // -> Four channels uncompressed
        case PixelFormat::RGBA8Unorm:
            iformat = GL_RGBA8;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            return;
        case PixelFormat::RGBA16Float:
            iformat = GL_RGBA16F;
            format = GL_RGBA;
            type = GL_FLOAT;
            return;
        case PixelFormat::RGBA32Float:
            iformat = GL_RGBA32F;
            format = GL_RGBA;
            type = GL_FLOAT;
            return;

        // Depth formats
        case PixelFormat::D16Unorm:
            iformat = GL_DEPTH_COMPONENT;
            format = GL_DEPTH_COMPONENT;
            type = GL_UNSIGNED_SHORT;
            return;
        case PixelFormat::D32Float:
            iformat = GL_DEPTH_COMPONENT;
            format = GL_DEPTH_COMPONENT;
            type = GL_UNSIGNED_SHORT;
            return;
        case PixelFormat::D24UNorm_S8Uint:
            iformat = GL_DEPTH_STENCIL;
            format = GL_DEPTH_STENCIL;
            type = GL_UNSIGNED_INT_24_8;
            return;

        default:
            CL_WARN("Failed to translate format to OpenGL4.5, not supported: ", (uint32)format);
        }
    }

    rsize ogl_compute_stride(PixelFormat format, rsize w)
    {
        switch (format)
        {
        case PixelFormat::Unknown:
            CAMY_ASSERT(false); // DXGI_FORMAT_UNKNOWN ?

        // Compressed formats
        case PixelFormat::BC1Unorm:
        case PixelFormat::BC3Unorm:
        case PixelFormat::BC5Unorm:
            CAMY_ASSERT(false); // TODO
        default:
            return 0;
        }
    }

    HResource RenderContext::create_surface(const SurfaceDesc& desc,
                                            const SubSurface* subsurfaces,
                                            rsize num_subsurfaces,
                                            const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        OpenGL4::flush_errors();

        if (desc.type == SurfaceDesc::Type::Surface2DArray ||
            desc.type == SurfaceDesc::Type::SurfaceCubeArray)
        {
            CL_ERR("OpenGL4.5 backend hasn't implemented texture arrays");
            return HResource::make_invalid();
        }

        if (desc.mip_levels == 0)
        {
            CL_ERR("Invalid argument: SurfaceDesc::mip_levels is 0");
            return HResource::make_invalid();
        }

        rsize expected_num_subsurfaces = desc.mip_levels;
        if (desc.type == SurfaceDesc::Type::SurfaceCube) expected_num_subsurfaces *= 6;

        if (subsurfaces != nullptr && expected_num_subsurfaces != num_subsurfaces)
        {
            CL_ERR("Invalid argument: SurfaceDesc::num_subsurfaces expected: ",
                   expected_num_subsurfaces, " but got: ", num_subsurfaces);
            return HResource::make_invalid();
        }

        bool failed = false;
        GLuint texture;
        glGenTextures(1, &texture);
        if (desc.type == SurfaceDesc::Type::SurfaceCube)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

            for (rsize f = 0; f < 6; ++f)
            {
                uint16 w = desc.width;
                uint16 h = desc.height;

                for (rsize m = 0; m < desc.mip_levels; ++m)
                {
                    rsize ss_idx = f * desc.mip_levels + m;
                    const void* cur_data =
                        subsurfaces == nullptr ? nullptr : subsurfaces[ss_idx].data;

                    GLenum iformat, format, type;
                    ogl_to_camy(desc.pixel_format, iformat, format, type);
                    if (ogl_is_compressed(desc.pixel_format))
                    {
                        GLuint num_bytes = ogl_compute_stride(desc.pixel_format, w) * w * h;
                        glCompressedTexImage2D(GL_PROXY_TEXTURE_2D, m, iformat, w, h, 0, num_bytes,
                                               cur_data);
                    }
                    else
                    {
                        glTexImage2D(GL_TEXTURE_2D, m, iformat, w, h, 0, format, type, cur_data);
                    }
                    failed = OpenGL4::has_errors() ? true : failed;

                    // TODO: Check for non-power of two
                    w /= 2;
                    h /= 2;
                }
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, texture);

            uint16 w = desc.width;
            uint16 h = desc.height;
            for (rsize m = 0; m < desc.mip_levels; ++m)
            {
                const void* cur_data = subsurfaces == nullptr ? nullptr : subsurfaces[m].data;

                GLenum iformat, format, type;
                ogl_to_camy(desc.pixel_format, iformat, format, type);
                if (ogl_is_compressed(desc.pixel_format))
                {
                    GLuint num_bytes = ogl_compute_stride(desc.pixel_format, w) * w * h;
                    glCompressedTexImage2D(GL_PROXY_TEXTURE_2D, m, iformat, w, h, 0, num_bytes,
                                           cur_data);
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_2D, m, iformat, w, h, 0, format, type, cur_data);
                }
                failed = OpenGL4::has_errors() ? true : failed;

                // TODO: Check for non-power of two
                w /= 2;
                h /= 2;
            }
        }

        if (failed)
        {
            glDeleteTextures(1, &texture);
            CL_ERR("Failed to create Texture, see above for more details");
            return HResource::make_invalid();
        }

        glObjectLabel(GL_TEXTURE, texture, API::strlen(name), name);
        HResource ret = m_resource_manager.allocate<Surface>();
        Surface& res = m_resource_manager.get<Surface>(ret);

        res.desc = desc;
        res.native.texture = texture;
        GLenum iformat, format, type;
        ogl_to_camy(desc.pixel_format, iformat, format, type);
        res.native.iformat = iformat;

        CL_INFO("Created Surface: ", name, "[", desc.width, "x", desc.height, "]");
        return ret;
    }

    HResource
    RenderContext::create_buffer(const BufferDesc& desc, const void* data, const char8* name)
    {
        // SSBO
        return HResource::make_invalid();
    }

    HResource RenderContext::create_vertex_buffer(const VertexBufferDesc& desc,
                                                  const void* data,
                                                  const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        OpenGL4::flush_errors();

        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, desc.num_elements * desc.element_size, data,
                     desc.usage == Usage::Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

        if (OpenGL4::has_errors())
        {
            glDeleteBuffers(1, &buffer);
            CL_ERR("Failed to create VertexBuffer, see above for more details");
            return HResource::make_invalid();
        }

        HResource ret = m_resource_manager.allocate<VertexBuffer>();
        VertexBuffer& res = m_resource_manager.get<VertexBuffer>(ret);

        glObjectLabel(GL_BUFFER, buffer, API::strlen(name), name);
        res.desc = desc;
        res.native.buffer = buffer;

        CL_INFO("Created VertexBuffer: ", name);

        return ret;
    }

    HResource RenderContext::create_index_buffer(const IndexBufferDesc& desc,
                                                 const void* data,
                                                 const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        OpenGL4::flush_errors();

        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     desc.num_elements * desc.extended32 ? sizeof(uint32) : sizeof(uint16), data,
                     desc.usage == Usage::Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

        if (OpenGL4::has_errors())
        {
            glDeleteBuffers(1, &buffer);
            CL_ERR("Failed to create IndexBuffer, see above for more details");
            return HResource::make_invalid();
        }

        glObjectLabel(GL_BUFFER, buffer, API::strlen(name), name);
        HResource ret = m_resource_manager.allocate<IndexBuffer>();
        IndexBuffer& res = m_resource_manager.get<IndexBuffer>(ret);

        res.desc = desc;
        res.native.buffer = buffer;

        CL_INFO("Created IndexBuffer: ", name);

        return ret;
    }

    HResource RenderContext::create_constant_buffer(const ConstantBufferDesc& desc,
                                                    const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        OpenGL4::flush_errors();

        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);
        glBufferData(GL_UNIFORM_BUFFER, desc.size, nullptr, GL_DYNAMIC_DRAW);

        if (OpenGL4::has_errors())
        {
            glDeleteBuffers(1, &buffer);
            CL_ERR("Failed to create ConstantBuffer, see above for more details");
            return HResource::make_invalid();
        }

        glObjectLabel(GL_BUFFER, buffer, API::strlen(name), name);
        HResource ret = m_resource_manager.allocate<ConstantBuffer>();
        ConstantBuffer& res = m_resource_manager.get<ConstantBuffer>(ret);

        res.desc = desc;
        res.native.buffer = buffer;

        CL_INFO("Successfully created RasterizerState: ", name);

        return ret;
    }

    HResource RenderContext::create_blend_state(const BlendStateDesc& desc, const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        return HResource::make_invalid();
    }

    HResource RenderContext::create_rasterizer_state(const RasterizerStateDesc& desc,
                                                     const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;

        HResource ret = m_resource_manager.allocate<RasterizerState>();
        RasterizerState& rs = m_resource_manager.get<RasterizerState>(ret);
        rs.desc = desc;

        CL_INFO("Successfully created RasterizerState: ", name);

        return ret;
    }

    HResource RenderContext::create_input_signature(InputSignatureDesc& desc, const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;

        // Actual initialization is delayed
        HResource ret = m_resource_manager.allocate<InputSignature>();
        InputSignature& is = m_resource_manager.get<InputSignature>(ret);
        is.desc = desc;

        CL_INFO("Successfully created InputSignature: ", name);

        return ret;
    }

    HResource RenderContext::create_sampler(const SamplerDesc& desc, const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        OpenGL4::flush_errors();

        GLuint sampler;
        glGenSamplers(1, &sampler);

        if (desc.filter == SamplerDesc::Filter::Point)
        {
            glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        }
        else
        {
            glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }

        if (desc.filter == SamplerDesc::Filter::Anisotropic)
        {
            GLfloat anisotropic_lvl = 0.f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropic_lvl);
            glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropic_lvl);
        }

        if (desc.comparison != SamplerDesc::Comparison::Never)
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        if (desc.comparison == SamplerDesc::Comparison::Less)
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        else if (desc.comparison == SamplerDesc::Comparison::LessEqual)
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        if (OpenGL4::has_errors())
        {
            CL_ERR("Failed to create sampler, see above for more details");
            return HResource::make_invalid();
        }

        glObjectLabel(GL_SAMPLER, sampler, API::strlen(name), name);
        HResource ret = m_resource_manager.allocate<Sampler>();
        Sampler& res = m_resource_manager.get<Sampler>(ret);

        res.desc = desc;
        res.native.sampler = sampler;

        CL_INFO("Created Sampler: ", name);

        return ret;
    }

    HResource RenderContext::create_depth_stencil_state(const DepthStencilStateDesc& desc,
                                                        const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;

        HResource ret = m_resource_manager.allocate<DepthStencilState>();
        DepthStencilState& dss = m_resource_manager.get<DepthStencilState>(ret);
        dss.desc = desc;

        CL_INFO("Successfully created DepthStencilState: ", name);

        return ret;
    }

    GLenum camy_to_opengl(ShaderDesc::Type type)
    {
        switch (type)
        {
        case ShaderDesc::Type::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderDesc::Type::Pixel:
            return GL_FRAGMENT_SHADER;
        default:
            return GL_NONE;
        }
    }

    HResource RenderContext::create_shader(const ShaderDesc& desc, const char8* name)
    {
        CAMY_ASSERT(m_data.is_valid());
        CAMY_ASSERT(id_for_current() != API::INVALID_CONTEXT_ID);
        CAMY_CHECK_CONTEXT_FOR_THREAD;
        OpenGL4::flush_errors();

        GLuint shader = glCreateShader(camy_to_opengl(desc.type));
        glShaderSource(shader, 1, (GLchar**)&desc.bytecode.data, (GLint*)&desc.bytecode.size);
        glCompileShader(shader);

        GLint cs;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &cs);
        if (cs == GL_FALSE)
        {
            GLint log_len;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
            RawAutoPtr<GLchar> log_msg = (GLchar*)API::allocate(CAMY_UALLOC((rsize)log_len));
            glGetShaderInfoLog(shader, log_len, &log_len, log_msg);

            CL_ERR("OpenGL::glCompileShader failed: ");
            CL_ERR((GLchar*)log_msg);

            glDeleteShader(shader);
            return HResource::make_invalid();
        }
        glDeleteShader(shader);
        GLuint program =
            glCreateShaderProgramv(camy_to_opengl(desc.type), 1, (GLchar**)&desc.bytecode.data);

        if (OpenGL4::has_errors())
        {
            CL_ERR("Failed to create shader: ", name);
            return HResource::make_invalid();
        }

        glObjectLabel(GL_PROGRAM, shader, API::strlen(name), name);
        HResource ret = m_resource_manager.allocate<Shader>();
        Shader& res = m_resource_manager.get<Shader>(ret);

        res.desc = desc;
        res.native.shader = program;

        CL_INFO("Created Shader: ", name);
        return ret;
    }

    void RenderContext::destroy_surface(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint texture = get_surface(handle).native.texture;
        if (glIsTexture(texture)) glDeleteTextures(1, &texture);
        m_resource_manager.deallocate<Surface>(handle);
    }

    void RenderContext::destroy_buffer(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint buffer = get_buffer(handle).native.buffer;
        if (glIsBuffer(buffer)) glDeleteBuffers(1, &buffer);
        m_resource_manager.deallocate<Buffer>(handle);
    }

    void RenderContext::destroy_vertex_buffer(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint buffer = get_vertex_buffer(handle).native.buffer;
        if (glIsBuffer(buffer)) glDeleteBuffers(1, &buffer);
        m_resource_manager.deallocate<VertexBuffer>(handle);
    }

    void RenderContext::destroy_index_buffer(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint buffer = get_index_buffer(handle).native.buffer;
        if (glIsBuffer(buffer)) glDeleteBuffers(1, &buffer);
        m_resource_manager.deallocate<IndexBuffer>(handle);
    }

    void RenderContext::destroy_constant_buffer(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint buffer = get_constant_buffer(handle).native.buffer;
        if (glIsBuffer(buffer)) glDeleteBuffers(1, &buffer);
        m_resource_manager.deallocate<ConstantBuffer>(handle);
    }

    void RenderContext::destroy_blend_state(HResource handle)
    {
        if (handle.is_invalid()) return;
        m_resource_manager.deallocate<BlendState>(handle);
    }

    void RenderContext::destroy_rasterizer_state(HResource handle)
    {
        if (handle.is_invalid()) return;
        m_resource_manager.deallocate<RasterizerState>(handle);
    }

    void RenderContext::destroy_input_signature(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint vao = get_input_signature(handle).native.vao;
        if (glIsVertexArray(vao)) glDeleteVertexArrays(1, &vao);
        m_resource_manager.deallocate<InputSignature>(handle);
    }

    void RenderContext::destroy_sampler(HResource handle)
    {
        if (handle.is_invalid()) return;

        GLuint sampler = get_sampler(handle).native.sampler;
        if (glIsSampler(sampler)) glDeleteSamplers(1, &sampler);
        m_resource_manager.deallocate<Sampler>(handle);
    }

    void RenderContext::destroy_depth_stencil_state(HResource handle)
    {
        if (handle.is_invalid()) return;
        m_resource_manager.deallocate<DepthStencilState>(handle);
    }

    void RenderContext::destroy_shader(HResource handle)
    {
        if (handle.is_invalid()) return;
        GLuint shader = get_shader(handle).native.shader;
        if (glIsProgram(shader)) glDeleteProgram(shader);
        m_resource_manager.deallocate<Shader>(handle);
    }

    Surface& RenderContext::get_surface(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<Surface>(handle);
    }

    Buffer& RenderContext::get_buffer(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<Buffer>(handle);
    }

    VertexBuffer& RenderContext::get_vertex_buffer(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<VertexBuffer>(handle);
    }

    IndexBuffer& RenderContext::get_index_buffer(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<IndexBuffer>(handle);
    }

    ConstantBuffer& RenderContext::get_constant_buffer(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<ConstantBuffer>(handle);
    }

    BlendState& RenderContext::get_blend_state(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<BlendState>(handle);
    }

    RasterizerState& RenderContext::get_rasterizer_state(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<RasterizerState>(handle);
    }

    InputSignature& RenderContext::get_input_signature(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<InputSignature>(handle);
    }

    Sampler& RenderContext::get_sampler(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<Sampler>(handle);
    }

    DepthStencilState& RenderContext::get_depth_stencil_state(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<DepthStencilState>(handle);
    }

    Shader& RenderContext::get_shader(HResource handle)
    {
        CAMY_ASSERT(handle.is_valid());
        return m_resource_manager.get<Shader>(handle);
    }
}

#endif