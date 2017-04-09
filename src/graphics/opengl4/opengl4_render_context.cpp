/* render_context.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/render_context.hpp>

#if defined(camy_backend_opengl4)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace camy
{
	HWND create_window(const StartupInfo& info, bool is_registered = false);
	void destroy_window(HWND hwnd);

	RenderContext::RenderContext() { } 

	RenderContext::~RenderContext() { destroy();  }

	bool RenderContext::init(const StartupInfo& info_in, RuntimeInfo& info_out)
	{
		destroy();

		HWND dummy_hwnd = create_window(info_in);

		if (!dummy_hwnd)
		{
			cl_internal_err("Failed to create dummy window");
			return false;
		}

		cl_info("Creating RenderContext..");
		cl_info("Num concurrent contexts: ", kMaxConcurrentContexts);
	
		HDC dummy_hdc = GetDC(dummy_hwnd);

		// Dummy pixelformat
		PIXELFORMATDESCRIPTOR dummy_pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,
			8,
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int dummy_pf = ChoosePixelFormat(dummy_hdc, &dummy_pfd);
		if (dummy_pf == 0)
		{
			cl_internal_err("Failed to find matching pixel format");
			return false;
		}
		SetPixelFormat(dummy_hdc, dummy_pf, &dummy_pfd);
		HGLRC dummy_hrc = wglCreateContext(dummy_hdc);
		wglMakeCurrent(dummy_hdc, dummy_hrc);

		// Loading extensions
		glewExperimental = true;
		if (glewInit() != GLEW_OK)
		{
			cl_internal_err("Failed to initialize glew");
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
			cl_internal_err("Failed to create window");
			return false;
		}

		HDC hdc = GetDC(hwnd);

		const int attribs[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
			WGL_SAMPLES_ARB, info_in.msaa,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, info_in.is_srgb,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
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
		int ctx_attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			WGL_CONTEXT_FLAGS_ARB, flags,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		m_data.hrc = wglCreateContextAttribsARB(hdc, nullptr, ctx_attribs);
		if (!m_data.hrc)
		{
			cl_system_err("WGLEW::CreateContextAttribsARB", GetLastError(), "");
			ReleaseDC(hwnd, hdc);
			destroy_window(hwnd);
			return false;
		}
		wglMakeCurrent(hdc, m_data.hrc);

		RECT client_rect;
		GetClientRect(hwnd, &client_rect);
		m_data.surface.desc.width = client_rect.right - client_rect.left;
		m_data.surface.desc.height = client_rect.bottom - client_rect.top;

		return true;
	}

	void RenderContext::destroy()
	{

	}

	void RenderContext::release(ContextID ctx_id)
	{

	}

	bool RenderContext::aquire(ContextID ctx_id)
	{
		return false;
	}

	ContextID RenderContext::get_id_for_current()
	{
		return kInvalidContextID;
	}

	RenderContextData& RenderContext::get_platform_data()
	{
		return m_data;
	}

	void RenderContext::flush(CommandList& command_list)
	{
	
	}

	void RenderContext::immediate_clear()
	{
	
	}

	void RenderContext::clear_color(HResource target, const float4& color)
	{
	
	}

	void RenderContext::clear_depth(HResource target, float depth, sint32 stencil)
	{
	
	}

	void RenderContext::immediate_cbuffer_update(HResource handle, void* data)
	{

	}

	HResource RenderContext::get_backbuffer_handle() const
	{
		return m_backbuffer_handle;
	}

	Surface& RenderContext::get_backbuffer()
	{
		return m_data.surface;
	}

	void  RenderContext::swap_buffers()
	{
	}

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
			camy_assert(false); // DXGI_FORMAT_UNKNOWN ?

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
			camy_assert(false);

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
			cl_warn("Failed to translate format to OpenGL4.5, not supported: ", (uint32)format);		
		}
	}

	rsize ogl_compute_stride(PixelFormat format, rsize w)
	{
		switch (format)
		{
		case PixelFormat::Unknown:
			camy_assert(false); // DXGI_FORMAT_UNKNOWN ?

		// Compressed formats
		case PixelFormat::BC1Unorm:
		case PixelFormat::BC3Unorm:
		case PixelFormat::BC5Unorm:
			camy_assert(false); // TODO
		default:
			return 0;
		}
	}

	HResource RenderContext::create_surface(const SurfaceDesc& desc, const SubSurface* subsurfaces, rsize num_subsurfaces, const char8* name)
	{
		if (desc.type == SurfaceDesc::Type::Surface2DArray ||
			desc.type == SurfaceDesc::Type::SurfaceCubeArray)
		{
			cl_internal_err("OpenGL4.5 backend hasn't implemented texture arrays");
			return HResource::make_invalid();
		}

		if (desc.mip_levels == 0)
		{
			cl_invalid_arg(desc.mip_levels);
			return HResource::make_invalid();
		}

		rsize expected_num_subsurfaces = desc.mip_levels;
		if (desc.type == SurfaceDesc::Type::SurfaceCube)
			expected_num_subsurfaces *= 6;

		if (subsurfaces != nullptr &&
			expected_num_subsurfaces != num_subsurfaces)
		{
			cl_invalid_arg_range(num_subsurfaces,expected_num_subsurfaces, expected_num_subsurfaces);
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
					const void* cur_data = subsurfaces == nullptr ? nullptr : subsurfaces[ss_idx].data;

					GLenum iformat, format, type;
					ogl_to_camy(desc.pixel_format, iformat, format, type);
					if (ogl_is_compressed(desc.pixel_format))
					{
						GLuint num_bytes = ogl_compute_stride(desc.pixel_format, w) * w * h;
						glCompressedTexImage2D(GL_PROXY_TEXTURE_2D,
							m, iformat, w, h, 0, num_bytes, cur_data);
					}
					else
					{
						glTexImage2D(GL_TEXTURE_2D,
							m, iformat, w, h, 0, format, type, cur_data);
					}
					failed = gl_err() ? true : failed;

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
					glCompressedTexImage2D(GL_PROXY_TEXTURE_2D,
						m, iformat, w, h, 0, num_bytes, cur_data);
				}
				else
				{
					glTexImage2D(GL_TEXTURE_2D,
						m, iformat, w, h, 0, format, type, cur_data);
				}
				failed = gl_err() ? true : failed;
				
				// TODO: Check for non-power of two
				w /= 2;
				h /= 2;
			}
		}

		if (failed)
		{
			glDeleteTextures(1, &texture);
			cl_internal_err("Failed to create Texture, see above for more details");
			return HResource::make_invalid();
		}

		HResource ret = m_resource_manager.allocate<Surface>(camy_loc, name);
		Surface& res = m_resource_manager.get<Surface>(ret);
		
		res.desc = desc;
		res.native.texture = texture;

		cl_info("Created Surface: ", name, "[", desc.width, "x", desc.height, "]");
		return ret;
	}

	HResource RenderContext::create_buffer(const BufferDesc& desc, const void* data, const char8* name)
	{
		return HResource::make_invalid();
	}

	HResource RenderContext::create_vertex_buffer(const VertexBufferDesc& desc, const void* data, const char8* name)
	{
		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, desc.num_elements * desc.element_size,
			data, desc.usage == Usage::Dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

		if (gl_err())
		{
			glDeleteBuffers(1, &buffer);
			cl_internal_err ("Failed to create VertexBuffer, see above for more details");
			return HResource::make_invalid();
		}

		HResource ret = m_resource_manager.allocate<VertexBuffer>(camy_loc, name);
		VertexBuffer& res = m_resource_manager.get<VertexBuffer>(ret);

		res.desc = desc;
		res.native.buffer = buffer;
		
		cl_info("Created VertexBuffer: ", name);

		return ret;
	}

	HResource RenderContext::create_index_buffer(const IndexBufferDesc& desc, const void* data, const char8* name)
	{
		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, desc.num_elements * desc.extended32 ? sizeof(uint32) : sizeof(uint16),
			data, desc.usage == Usage::Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

		if (gl_err())
		{
			glDeleteBuffers(1, &buffer);
			cl_err("Failed to create IndexBuffer, see above for more details");
			return HResource::make_invalid();
		}

		HResource ret = m_resource_manager.allocate<IndexBuffer>(camy_loc, name);
		IndexBuffer& res = m_resource_manager.get<IndexBuffer>(ret);

		res.desc = desc;
		res.native.buffer = buffer;

		cl_info("Created IndexBuffer: ", name);

		return ret;
	}

	HResource RenderContext::create_constant_buffer(const ConstantBufferDesc& desc, const char8* name)
	{
		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, buffer);
		glBufferData(GL_UNIFORM_BUFFER, desc.size, nullptr, GL_DYNAMIC_DRAW);

		if (gl_err())
		{
			glDeleteBuffers(1, &buffer);
			cl_internal_err("Failed to create ConstantBuffer, see above for more details");
			return HResource::make_invalid();
		}

		HResource ret = m_resource_manager.allocate<ConstantBuffer>(camy_loc, name);
		ConstantBuffer& res = m_resource_manager.get<ConstantBuffer>(ret);

		res.desc = desc;
		res.native.buffer = buffer;

		cl_info("Successfully created constant buffer: ", name);

		return ret;
	}

	HResource RenderContext::create_blend_state(const BlendStateDesc& desc, const char8* name)
	{
		return HResource::make_invalid();
	}

	HResource RenderContext::create_rasterizer_state(const RasterizerStateDesc& desc, const char8* name)
	{
		return HResource::make_invalid();
	}

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

	HResource RenderContext::create_input_signature(const InputSignatureDesc& desc, const char8* name)
	{
		GLuint vao;
		glGenVertexArrays(1, &vao);

		//http://stackoverflow.com/questions/21153729/glbindvertexbuffer-vs-glbindbuffer
		// TODO: Support multiple bindings
		int offset = 0;		
		for (rsize i = 0; i < desc.num_elements; ++i)
		{
			InputElement& element = desc.elements[i];
			glVertexArrayVertexAttribFormatEXT(vao,
				0,
				camy_to_opengl_comps(element.type),
				camy_to_opengl_type(element.type),
				GL_FALSE, offset);
		}

		return HResource::make_invalid();
	}

	HResource RenderContext::create_sampler(const SamplerDesc& desc, const char8* name)
	{
		return HResource::make_invalid();
	}

	HResource RenderContext::create_depth_stencil_state(const DepthStencilStateDesc& desc, const char8* name)
	{
		return HResource::make_invalid();
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
		GLuint shader =	glCreateShader(camy_to_opengl(desc.type));
		glShaderSource(shader, 1, (const GLchar**)&desc.bytecode.data, nullptr);
		glCompileShader(shader);

		GLint cs;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &cs);
		if (cs == GL_FALSE)
		{
			GLint log_len;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
			RawAutoPtr<GLchar> log_msg = (GLchar*)allocate(camy_loc, log_len);
			glGetShaderInfoLog(shader, log_len, &log_len, log_msg);

			cl_system_err("OpenGL4.5", "glCompileShader", "");
			cl_internal_err((GLchar*)log_msg);

			glDeleteShader(shader);
			return HResource::make_invalid();
		}

		HResource ret = m_resource_manager.allocate<Shader>(camy_loc, name);
		Shader& res = m_resource_manager.get<Shader>(ret);

		res.desc = desc;
		res.native.shader = ret;
		
		cl_info("Created Shader: ", name);
		return ret;
	}

	void RenderContext::destroy_surface(HResource handle)
	{

	}

	void RenderContext::destroy_buffer(HResource handle)
	{

	}

	void RenderContext::destroy_vertex_buffer(HResource handle)
	{

	}

	void RenderContext::destroy_index_buffer(HResource handle)
	{

	}

	void RenderContext::destroy_constant_buffer(HResource handle)
	{

	}

	void RenderContext::destroy_blend_state(HResource handle)
	{
	}

	void RenderContext::destroy_rasterizer_state(HResource handle)
	{
	}

	void RenderContext::destroy_input_signature(HResource handle)
	{
	}

	void RenderContext::destroy_sampler(HResource handle)
	{
	}

	void RenderContext::destroy_depth_stencil_state(HResource handle)
	{
	}

	void RenderContext::destroy_shader(HResource handle)
	{
	}

	Surface& RenderContext::get_surface(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<Surface>(handle);
	}

	Buffer& RenderContext::get_buffer(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<Buffer>(handle);
	}

	VertexBuffer& RenderContext::get_vertex_buffer(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<VertexBuffer>(handle);
	}

	IndexBuffer& RenderContext::get_index_buffer(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<IndexBuffer>(handle);
	}

	ConstantBuffer& RenderContext::get_constant_buffer(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<ConstantBuffer>(handle);
	}

	BlendState& RenderContext::get_blend_state(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<BlendState>(handle);
	}

	RasterizerState& RenderContext::get_rasterizer_state(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<RasterizerState>(handle);
	}

	InputSignature& RenderContext::get_input_signature(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<InputSignature>(handle);
	}

	Sampler& RenderContext::get_sampler(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<Sampler>(handle);
	}

	DepthStencilState& RenderContext::get_depth_stencil_state(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<DepthStencilState>(handle);
	}

	Shader& RenderContext::get_shader(HResource handle)
	{
		camy_assert(handle.is_valid());
		return m_resource_manager.get<Shader>(handle);
	}
}

#endif