/* opengl_platform.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics/platform.hpp>

#if defined(camy_os_windows) && defined(camy_backend_opengl4)

// camy
#include <camy/graphics/render_context.hpp>
#include <camy/core/memory/alloc.hpp>

// OGL
#include <GL/glew.h>
#include <GL/wglew.h>

// Win32
#include <Windows.h>

extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

namespace camy
{
	namespace
	{
		RuntimeInfo		 g_runtime_info;
		RenderContext*	 g_render_context = nullptr;
		WindowDesc::Proc g_custom_winproc = nullptr;
	}

	void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* unused)
	{
#if defined(_MSC_VER)
		OutputDebugStringA("[camy::OpenGL4.5] ");
		OutputDebugStringA(message);
		OutputDebugStringA("\n");
#else
#	error Implement this, just log
#endif
	}

	struct ProcData
	{
		HWND	hwnd;
		UINT	msg;
		WPARAM  wparam;
		LPARAM  lparam;
	};

	LRESULT g_winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (g_custom_winproc != nullptr)
		{
			ProcData pc{ hwnd, msg, wparam, lparam };
			g_custom_winproc(&pc);
		}

		return DefWindowProcA(hwnd, msg, wparam, lparam);
	}

	HWND create_window(const StartupInfo& info, bool is_registered = false)
	{
		const char* class_name = "camy@d3d11.1@classname";

		WNDCLASSEXA wc;
		HINSTANCE cur_instance = GetModuleHandle(NULL);

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc = g_winproc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = cur_instance;
		wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wc.hIconSm = wc.hIcon;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = class_name;
		wc.cbSize = sizeof(WNDCLASSEXA);

		if (!is_registered)
		{
			if (!RegisterClassExA(&wc))
			{
				cl_internal_err("Failed to Win32::RegisterClassEx");
				return false;
			}
		}

		float screen_width = (float)GetSystemMetrics(SM_CXVIRTUALSCREEN);
		float screen_height = (float)GetSystemMetrics(SM_CYVIRTUALSCREEN);

		DWORD style = WS_OVERLAPPEDWINDOW;
		if (info.window.style == WindowDesc::Style::Popup)
			style = WS_POPUP;

		HWND ret_handle = CreateWindowA(class_name, info.window.name, style,
			CW_USEDEFAULT, CW_USEDEFAULT, info.window.width, info.window.height, NULL, NULL, cur_instance, NULL);
		if (ret_handle == NULL)
		{
			cl_internal_err("Failed to Win32::CreateWindow with error: ", GetLastError());
			return nullptr;
		}

		ShowWindow(ret_handle, SW_SHOW);

		g_custom_winproc = info.window.proc;

		return ret_handle;
	}

	void destroy_window(HWND hwnd)
	{
		DestroyWindow(hwnd);
	}

	namespace API
	{
		bool startup(const StartupInfo& info)
		{
			g_render_context = tallocate<RenderContext>(camy_loc, 16);
			if (g_render_context == nullptr)
			{
				cl_internal_err("Failed to startup camy");
				return false;
			}

			if (!g_render_context->init(info, g_runtime_info))
			{
				tdeallocate(g_render_context);
				return false;
			}

			glDebugMessageCallback(gl_debug_callback, nullptr);

			cl_info("Successfully started up camy, enjoy");
			return true;
		}

		RenderContext& render_context()
		{
			return *g_render_context;
		}

		RenderContext& rc()
		{
			return *g_render_context;
		}

		const RuntimeInfo& runtime_info()
		{
			return g_runtime_info;
		}

		const RuntimeInfo& info()
		{
			return g_runtime_info;
		}

		rsize query(Query query)
		{
			return 0;
		}

		float4x4 to_shader_order(const float4x4& mat)
		{
			return mat;
		}
	}
}
#endif