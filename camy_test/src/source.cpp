// Camy
#include <camy.hpp>
#include <camy_render.hpp>
#include <camy_importer.hpp>

// C++ STL
#include <random>
#include <chrono>
#include <algorithm>
#include <vector>

// Simple helper that creates a window
#include "../include/window.hpp"

// Globals
HWND			g_window_handle;
camy::Surface*  g_window_surface;

// Control's configuration
const float				g_mouse_sensibility{ 0.1f };

void control_camera(camy::Camera& camera, float elapsed_ms)
{
	const float step{ 10.f * elapsed_ms / 1000.f };
	auto delta_x{ 0.f };
	auto delta_y{ 0.f };

	if (GetFocus() == g_window_handle)
	{
		RECT window_rect;
		GetWindowRect(g_window_handle, &window_rect);
		auto width{ static_cast<float>(window_rect.right - window_rect.left) };
		auto height{  static_cast<float>(window_rect.bottom - window_rect.top) };

		POINT current_mouse_pos;
		GetCursorPos(&current_mouse_pos);
		ScreenToClient(g_window_handle, &current_mouse_pos);
		delta_x = current_mouse_pos.x - width / 2;
		delta_y = current_mouse_pos.y - height / 2;

		POINT new_mouse_position;
		new_mouse_position.x = static_cast<LONG>(width) / 2;
		new_mouse_position.y = static_cast<LONG>(height) / 2;
		ClientToScreen(g_window_handle, &new_mouse_position);
		SetCursorPos(new_mouse_position.x, new_mouse_position.y);
	}

	if (GetAsyncKeyState(0x57) != 0)
		camera.move(step);

	if (GetAsyncKeyState(0x53) != 0)
		camera.move(-step);

	if (GetAsyncKeyState(0x41) != 0)
		camera.strafe(step);

	if (GetAsyncKeyState(0x44) != 0)
		camera.strafe(-step);

	if (GetAsyncKeyState(0x20) != 0)
		camera.fly(step);

	if (GetAsyncKeyState(0xA0) != 0)
		camera.fly(-step);

	delta_x *= g_mouse_sensibility;
	camera.yaw(delta_x * DirectX::XM_PI / 180);

	delta_y *= g_mouse_sensibility;
	camera.pitch(-delta_y * DirectX::XM_PI / 180);
}

int at_exit(int code)
{
	// Show console and wait for input to leave
	auto chandle{ GetConsoleWindow() };
	SetFocus(chandle);
	std::cout << "Press Enter to exit..." << std::endl;
	std::cin.get();
	return code;
}

int main(int argc, char* argv[])
{
	using namespace camy;
	using namespace allocators;
	using namespace DirectX;

	camy::init();
	GPUBackend& gpu = hidden::gpu;

	// Creating window
	g_window_handle = create_window();
	g_window_surface = gpu.create_window_surface(g_window_handle);
	if (!g_window_surface)
		return at_exit(EXIT_FAILURE);

	// Importing scene
	Scene scene(gpu);
	ImportedScene iscene;
	if (!iscene.load("sample_scene.metadata", scene))
		return at_exit(EXIT_FAILURE);

	// Setting camera
	Camera camera(1920.f / 1080);

	// Counters
	auto start{ std::chrono::high_resolution_clock::now() };
	auto elapsed{ 0.f };
	auto counter{ 0.f };
	auto frames{ 0u };

	// Loading rendered
	Renderer renderer;
	if (!renderer.load(g_window_surface, camera.get_projection(), PostProcessPipeline::Effects_HDR | PostProcessPipeline::Effects_Bloom))
		return at_exit(EXIT_FAILURE);

	// Setting sunlight
	/*
	scene.set_shadow_casting_light_enabled(true);
	auto& sun{ scene.get_shadow_casting_light() };
	sun.color = colors::white;
	sun.direction = { -1.f, -1.f, -1.f };
	sun.intensity = 1.f;
	*/

	// Output viewport ( Todo : implement default in the future ) 
	Viewport viewport;
	viewport.left = 0.f;
	viewport.right = 1920.f;
	viewport.top = 0.f;
	viewport.bottom = 1080.f;

	while (true)
	{
		peek_and_dispatch_msg();

		control_camera(camera, elapsed);
		
		// Renders the scene from the current camera
		renderer.render(scene, camera, viewport);

		// Waits the end of the frame and all operations, in addition it swaps buffers
		renderer.sync();

		auto end{ std::chrono::high_resolution_clock::now() };
		elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>((end - start)).count()) / 1000000;
		start = end;

		/* Frames per second counter */
		++frames;
		counter += elapsed;
		if (counter >= 1000.f)
		{	
			std::string title{ "camy_test | FPS:" + std::to_string(frames) + " |" };
			SetWindowTextA(g_window_handle, title.c_str());
			frames = 0u;
			counter = 0.f;
		}
	}

	return EXIT_SUCCESS;
}