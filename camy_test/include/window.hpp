#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

const unsigned int g_width{ 1280 };
const unsigned int g_height{ 720 };

inline LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_SETFOCUS:
		ShowCursor(false);
		break;

	case WM_KILLFOCUS:
		ShowCursor(true);
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

inline HWND create_window()
{
	WNDCLASSEX wc;
	HINSTANCE cur_instance = GetModuleHandle(NULL);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = wnd_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = cur_instance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"sample_class";
	wc.cbSize = sizeof(WNDCLASSEX);

	RegisterClassEx(&wc);

	auto screen_width{ static_cast<float>(GetSystemMetrics(SM_CXVIRTUALSCREEN)) };
	auto screen_height{ static_cast<float>(GetSystemMetrics(SM_CYVIRTUALSCREEN)) };

	HWND ret_handle = CreateWindow(L"sample_class", L"camy test", WS_OVERLAPPEDWINDOW,
		static_cast<UINT>(screen_width / 2 - g_width / 2), static_cast<UINT>(screen_height / 2 - g_height / 2), g_width, g_height, NULL, NULL, cur_instance, NULL);
	if (ret_handle == NULL)
	{
		return nullptr;
	}

	ShowWindow(ret_handle, SW_SHOW);

	return ret_handle;
}

inline void peek_and_dispatch_msg()
{
	MSG msg;
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}