// Header
#include <camy_render/post_process_pipeline.hpp>

// camy
#include <camy/camy_init.hpp>

// C++ STL
#include <algorithm>

#define BYTE camy::Byte
namespace shader_code
{
#include "shaders/to_luminance_ps.hpp"
#include "shaders/downsample_luminance_ps.hpp"
#include "shaders/downsample_bright_ps.hpp"
}
#undef BYTE

namespace camy
{
	PostProcessPipeline::~PostProcessPipeline()
	{
		dispose();
	}

	bool PostProcessPipeline::load(Surface* hdr_scene, Surface* output_rt)
	{
		auto width{ hdr_scene->description.width };
		auto height{ hdr_scene->description.height };
		if (width != output_rt->description.width ||
			height != output_rt->description.height)
		{
			camy_error("Failed to initialize postprocessing pipeline because entrance resolution differs from the output one");
			return false;
		}

		// In the first step when we go from the HDR scene to the luminance map, we downsample to a power of two, this makes
		// the dowsampling later on way easier and we avoid "losing" pixels at extremely low resolutions ( that would be very noticeable if we lose 1 out of 3 pixels say )
		// Should be -1 TODO ( upper_pow2 is broken )
		width = 2 << (math::upper_pow2(width) - 2);
		height = 2 << (math::upper_pow2(height) - 2);

		luminance_map = hidden::gpu.create_render_target(Surface::Format::R16Float, width, height);
		if (luminance_map == nullptr)
		{
			dispose();
			return false;
		}

		// Loading shaders
		if (!to_luminance_ps.load(Shader::Type::Pixel, shader_code::to_luminance_ps, sizeof(shader_code::to_luminance_ps))) { dispose(); return false; }
		if (!luminance_downsample_ps.load(Shader::Type::Pixel, shader_code::downsample_luminance_ps, sizeof(shader_code::downsample_luminance_ps))) { dispose(); return false; }
		if (!bright_downsample_ps.load(Shader::Type::Pixel, shader_code::downsample_bright_ps, sizeof(shader_code::downsample_bright_ps))) { dispose(); return false; }
		
		// Sampler is shared between all stages
		
		default_sampler = hidden::gpu.create_sampler(Sampler::Filter::Point, Sampler::Address::Clamp);
		if (default_sampler == nullptr)
		{
			dispose();
			return false;
		}

		downsample_luminance_sampler_parameter.shader_variable = luminance_downsample_ps.get("luminance_sampler");
		downsample_luminance_sampler_parameter.data = default_sampler;

		to_luminance_sampler_parameter.shader_variable = to_luminance_ps.get("surface_sampler");
		to_luminance_sampler_parameter.data = default_sampler;

		// First step is converting the RGB hdr rendered scene to the F16 luminance correspective
		PostProcessItem to_luminance_pp;
		to_luminance_pp.input_surface = hdr_scene;
		to_luminance_pp.output_surface = luminance_map;
		to_luminance_pp.pixel_shader = &to_luminance_ps;
		to_luminance_pp.parameters.num_parameters = 1;
		to_luminance_pp.parameters.parameters = &to_luminance_sampler_parameter;
		pp_items.push_back(to_luminance_pp);

		// Now we subsequently downsample the luminance map down to 1x1 RT
		auto next_x{ width }; 
		auto next_y{ height };
		Surface* next_input_rt{ luminance_map };
		while (next_x != 1 || next_y != 1)
		{
			auto down_x{ std::max(1u, next_x / 2) };
			auto down_y{ std::max(1u, next_y / 2) };

			Surface* mipmap = hidden::gpu.create_render_target(Surface::Format::R16Float, down_x, down_y);
			if (mipmap == nullptr)
			{
				dispose();
				return false;
			}

			luminance_mipmaps.push_back(mipmap);

			// Creating respective post process item
			PostProcessItem pp_item;
			pp_item.input_surface = next_input_rt;
			pp_item.output_surface = mipmap;
			pp_item.pixel_shader = &luminance_downsample_ps;
			pp_items.push_back(pp_item);
	
			pp_item.parameters.num_parameters = 1;
			pp_item.parameters.parameters = &downsample_luminance_sampler_parameter;

			next_x = down_x;
			next_y = down_y;
			next_input_rt = mipmap;
		}

		// Time to add bloom
		auto bright_downsample_width{ hdr_scene->description.width / 2 };
		auto bright_dowsnaple_height{ hdr_scene->description.height / 2 };
		downsampled_bright_scene = hidden::gpu.create_render_target(Surface::Format::RGBA16Float, bright_downsample_width, bright_dowsnaple_height);
		if (downsampled_bright_scene == nullptr)
		{
			dispose();
			return false;
		}

		PostProcessItem pp_item;
		pp_item.input_surface = hdr_scene;
		pp_item.output_surface = downsampled_bright_scene;
		pp_item.pixel_shader = &bright_downsample_ps;

		pp_items.push_back(pp_item);

		return true;
	}

	void PostProcessPipeline::dispose()
	{
		for (auto& mipmap : luminance_mipmaps)
			hidden::gpu.safe_dispose(mipmap);

		hidden::gpu.safe_dispose(downsampled_bright_scene);

		// TODO: Pls fix the whole constness stuff and then uncomment this line
	//	hidden::gpu.safe_dispose(luminance_sampler_parameter.sampler);

		pp_items.clear();

		hidden::gpu.safe_dispose(luminance_map);

		to_luminance_ps.unload();
		luminance_downsample_ps.unload();
		bright_downsample_ps.unload();
		blur_horizontal_ps.unload();
		blur_vertical_ps.unload();
		tone_map_ps.unload();
	}
}