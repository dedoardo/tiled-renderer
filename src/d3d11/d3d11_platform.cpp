/* d3d11_platform.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/graphics.hpp>

#if defined(CAMY_OS_WINDOWS) && defined(CAMY_BACKEND_D3D11)

// camy
#include <camy/render_context.hpp>

// win32
#include <Windows.h>

namespace camy
{
	namespace
	{
		RuntimeInfo		 g_runtime_info;
		RenderContext*	 g_render_context = nullptr;
		WindowDesc::Proc g_custom_winproc = nullptr;
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
				CL_ERR("Win32::RegisterClassEx failed with error: ", GetLastError());
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
			CL_ERR("Win32::CreateWindow failed with error: ", GetLastError());
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
			API::memory_init();
			API::log_init();

			if (info.window.style != WindowDesc::Style::None)
			{
				g_runtime_info.whandle = create_window(info);
				if (g_runtime_info.whandle == nullptr)
					return false;
				
				g_runtime_info.width = info.window.width;
				g_runtime_info.height = info.window.height;

			}

			g_render_context = API::tallocate<RenderContext>(CAMY_UALLOC1);
			if (g_render_context == nullptr)
			{
				CL_ERR("Failed to allocate RenderContext");
				return false;
			}

			if (!g_render_context->init(info, g_runtime_info))
			{
				CL_ERR("Failed to initialize D3D11RenderContext");
				tdeallocate(g_render_context);
				return false;
			}

			CL_INFO("Successfully started up camy, enjoy");
			return true;
		}

		void shutdown()
		{
			if (g_render_context == nullptr)
			{
				CL_WARN("Shutting down non-initialized camy");
				return;
			}

			if (g_runtime_info.whandle != nullptr)
				destroy_window((HWND)g_runtime_info.whandle);;

			g_render_context->destroy();

			API::tdeallocate(g_render_context);
			g_render_context = nullptr;
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
			switch (query)
			{
			case Query::ConstantByteSize:
				return 16; // 4 * 32-bit components
			case Query::MinConstantsPerUpdate:
				return 16; // Minimum 16 constants per range
			case Query::ShaderMatrixOrder:
				return (rsize)ShaderMatrixOrder::ColMajor;
			default:
				return -1;
			}
		}

		float4x4 to_shader_order(const float4x4& mat)
		{
			return transpose(mat);
		}
	}
}
#endif