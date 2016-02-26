// Header
#include <camy_render/post_process_pipeline.hpp>

// camy
#include <camy/init.hpp>

// C++ STL
#include <algorithm>

#define BYTE camy::Byte
namespace shader_code
{
#include "shaders/to_luminance_ps.hpp"
#include "shaders/downsample_luminance_ps.hpp"
#include "shaders/downsample_bright_ps.hpp"
#include "shaders/kawase_blur_ps.hpp"
#include "shaders/bloom_ps.hpp"
#include "shaders/tone_map_ps.hpp"
}
#undef BYTE

namespace camy
{
	const u32 PostProcessPipeline::kawase_blur_kernels[]{ 0u, 1u, 2u, 2u, 3u };
	static_assert(PostProcessPipeline::kawase_blur_iterations == (sizeof(PostProcessPipeline::kawase_blur_kernels) / sizeof(u32)), "Number of kernels must correspond to the number of iterations");

	PostProcessPipeline::~PostProcessPipeline()
	{
		dispose();
	}

	bool PostProcessPipeline::load(Surface* input_image, Surface* output_image, u32 effects)
	{
		dispose();

		/*
			No effects
		*/
		if (effects == Effects_None)
		{
			camy_info("Succesfully loaded empty postprocessing pipeline");
			return true;
		}

		/*
			Making sure they are not null
		*/
		if (input_image == nullptr || output_image == nullptr)
		{
			camy_error("Error: input_image != nullptr && output_image != nullptr");
			return false;
		}

		/*
			Input and output sizes have to match
		*/
		auto input_width{ input_image->description.width };
		auto input_height{ input_image->description.height };
		auto output_width{ output_image->description.width };
		auto output_height{ output_image->description.height };

		if (input_width != output_width || input_height != output_height)
		{
			camy_error("Error: Input and Output sizes have to match");
			return false;
		}

		/*
			Loading bilinear sampler that is shared by most effects
		*/
		bilinear_sampler = hidden::gpu.create_sampler(Sampler::Filter::Point, Sampler::Address::Clamp);
		if (bilinear_sampler == nullptr)
		{
			dispose();
			return false;
		}
		additive_blend_state = hidden::gpu.create_blend_state(BlendState::Mode::Additive);
		if (additive_blend_state == nullptr)
		{
			dispose();
			return false;
		}

		/*
			We are keeping track of the last effect that is applied and its output surface
			that will then be used in the merging step ( if there is any )
		*/
		Surface* pp_output_image{ input_image };

		/*
			Effects_HDR
			http://www.cmap.polytechnique.fr/~peyre/cours/x2005signal/hdr_photographic.pdf
		*/
		if (effects & Effects_HDR)
		{
			/*
				Loading required resources:
					- To luminance + downsample pixel shader
					- Downsample pixel shader
			*/
			if (!to_luminance_ps.load(Shader::Type::Pixel, shader_code::to_luminance_ps, sizeof(shader_code::to_luminance_ps))) 
			{ 
				camy_error("Error: Failed to load \"to luminance\" pixel shader");
				dispose(); 
				return false; 
			}
			if (!luminance_downsample_ps.load(Shader::Type::Pixel, shader_code::downsample_luminance_ps, sizeof(shader_code::downsample_luminance_ps))) 
			{ 
				camy_error("Error: Failed to load \"luminance downsample\" pixel shader");
				dispose(); 
				return false; 
			}

			// Also preparing parameters
			downsample_luminance_sampler_parameter.shader_variable = luminance_downsample_ps.get("luminance_sampler");
			downsample_luminance_sampler_parameter.data = bilinear_sampler;

			to_luminance_sampler_parameter.shader_variable = to_luminance_ps.get("surface_sampler");
			to_luminance_sampler_parameter.data = bilinear_sampler;

			/*
				First step in HDR is transforming the current RGB surface we have into a Luminance one, this and the first
				downsample are done at the same time
				Second step in HDR is getting the average downsampled luminance of the scene, we do this by downsampling
				using a bilinear filter. To simplify further downsampling, the first downsample will go to a power of two size.
				Todo: Should be -1 upper_pow2 is currently broken.
				Todo: Add multiple smampled to account for loss of precision
			*/
			auto downsampled_width{ static_cast<u32>(2 << (math::upper_pow2(input_width) - 2)) };
			auto downsampled_height{ static_cast<u32>(2 << (math::upper_pow2(input_height) - 2)) };

			luminance_map = hidden::gpu.create_render_target(Surface::Format::R16Float, downsampled_width, downsampled_height);
			if (luminance_map == nullptr)
			{
				dispose();
				return false;
			}

			PostProcessItem to_luminance_pp;
			to_luminance_pp.input_surface = input_image;
			to_luminance_pp.output_surface = luminance_map;
			to_luminance_pp.pixel_shader = &to_luminance_ps;
			to_luminance_pp.parameters.num_parameters = 1;
			to_luminance_pp.parameters.parameters = &to_luminance_sampler_parameter;
			pp_items.push_back(to_luminance_pp);

			// Now we subsequently downsample the luminance map down to 1x1 RT
			auto next_width{ downsampled_width };
			auto next_height{ downsampled_height };
			Surface* next_input_rt{ luminance_map };
			while (next_width != 1 || next_height != 1)
			{
				auto down_width{ std::max(1u, next_width / 2) };
				auto down_height{ std::max(1u, next_height / 2) };

				Surface* mipmap = hidden::gpu.create_render_target(Surface::Format::R16Float, down_width, down_height);
				if (mipmap == nullptr)
				{
					camy_warning("Error: Failed to create Effects_HDR mipmap level ", luminance_mipmaps.size());
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

				next_width = down_width;
				next_height = down_height;
				next_input_rt = mipmap;
			}

			/*
				Todo: not needed yet
				If any further postprocessing has to be done its output should not be in the
				output_image because otherwise sampling and using it as render target in the same
				drawcall wont be possibly. Everything needs to be output to an external surface
				that will then be sampled in the merge / tone map item.
				If HDR is not enabled then there wont be a merge step and the result of postprocessing
				can directly be the output_image.
				The render target is the same as the input surface
			*/
			if (effects != Effects_HDR)
			{
				offscreen_target = hidden::gpu.create_render_target(input_image->description.format, input_width, input_height);
				if (offscreen_target == nullptr)
				{
					camy_error("Error: Failed to create postprocessing offscreen target");
					dispose();
					return false;
				}

				pp_output_image = offscreen_target;
			}

			camy_info("Successfully loaded Effects_HDR");
		}

		if (effects & Effects_Bloom)
		{
			/*
				Loading required resources:
			*/
			if (!bright_downsample_ps.load(Shader::Type::Pixel, shader_code::downsample_bright_ps, sizeof(shader_code::downsample_bright_ps)))
			{
				camy_error("Error: Failed to load \"bright downsample\" pixel shader");
				dispose();
				return false;
			}

			if (!kawase_blur_ps.load(Shader::Type::Pixel, shader_code::kawase_blur_ps, sizeof(shader_code::kawase_blur_ps)))
			{
				camy_error("Error: Failed to load \"kawase blur\" pixel shader");
				dispose();
				return false;
			}

			if (!bloom_ps.load(Shader::Type::Pixel, shader_code::bloom_ps, sizeof(shader_code::bloom_ps)))
			{
				camy_error("Error: Failed to load \"bloom\" pixel shader");
				dispose();
				return false;
			}

			/*
				First step is downsampling and thresholding the scene based on brightness ( currently the threshold is hardcoded
				also creating the helper render target for blurring ( might find a way later on to reuse an already existing 
				render target.
			*/

			auto bright_downsampled_width{ input_width  };
			auto bright_downsampled_height{ input_height  };
			downsampled_bright_scene = hidden::gpu.create_render_target(input_image->description.format, bright_downsampled_width, bright_downsampled_height);
			if (downsampled_bright_scene == nullptr)
			{
				camy_error("Error: Failed to create bloom downsampled render target");
				dispose();
				return false;
			}

			kawase_blur_extra_rts[0] = hidden::gpu.create_render_target(input_image->description.format, bright_downsampled_width, bright_downsampled_height);
			kawase_blur_extra_rts[1] = hidden::gpu.create_render_target(input_image->description.format, bright_downsampled_width, bright_downsampled_height);
			if (kawase_blur_extra_rts[0] == nullptr ||
				kawase_blur_extra_rts[1] == nullptr)
			{
				camy_error("Error: Failed to create bloom extra render target");
				dispose();
				return false;
			}

			// Downsample + bright
			PostProcessItem downsample_bright_item;
			downsample_bright_item.input_surface = input_image;
			downsample_bright_item.output_surface = downsampled_bright_scene;
			downsample_bright_item.pixel_shader = &bright_downsample_ps;

			bright_downsample_params.shader_variable = bright_downsample_ps.get("bilinear_sampler");
			bright_downsample_params.data = bilinear_sampler;

			downsample_bright_item.parameters.parameters = &bright_downsample_params;
			downsample_bright_item.parameters.num_parameters = 1;

			pp_items.push_back(downsample_bright_item);
		
			Surface* next_output{ kawase_blur_extra_rts[1] };
			Surface* next_input{ downsampled_bright_scene };
			auto i{ 0u };
			for (; i < kawase_blur_iterations; ++i)
			{
				PostProcessItem kawase_blur_item;
				kawase_blur_item.input_surface = next_input;
			
				if (i == kawase_blur_iterations - 1) // We only do this in case we are not using HDR rts otherwise we might lose precision 
					kawase_blur_item.output_surface = (effects & Effects_HDR) ? pp_output_image : output_image;
				else
					kawase_blur_item.output_surface = next_output;

				kawase_blur_item.pixel_shader = &kawase_blur_ps;

				kawase_blur_args[i].iteration = kawase_blur_kernels[i];
				kawase_blur_args[i].texel_size = float2(1.f / next_output->description.width, 1.f / next_output->description.height);

				kawase_blur_params[i * 2 + 0].shader_variable = kawase_blur_ps.get("bilinear_sampler");
				kawase_blur_params[i * 2 + 0].data = bilinear_sampler;

				kawase_blur_params[i * 2 + 1].shader_variable = kawase_blur_ps.get(shaders::KawaseBlurArgs::name);
				kawase_blur_params[i * 2 + 1].data = &kawase_blur_args[i];

				kawase_blur_item.parameters.num_parameters = 2;
				kawase_blur_item.parameters.parameters = &kawase_blur_params[i * 2];

				pp_items.push_back(kawase_blur_item);
			
				next_input = next_output;
				next_output = kawase_blur_extra_rts[i % 2];
			}

			/*
				Last step is adding the blurred version ( contained in kawase_blur_extra rt ) 
				to the original version contained in input_image into the pp_output_image
			*/
			PostProcessItem bloom_item;
			bloom_item.input_surface = input_image;
			bloom_item.output_surface = pp_items.back().output_surface;
			bloom_item.pixel_shader = &bloom_ps;
			bloom_item.blend_state = additive_blend_state;

			bloom_params[0].shader_variable = bloom_ps.get("bilinear_sampler");
			bloom_params[0].data = bilinear_sampler;

			bloom_item.parameters.num_parameters = 1;
			bloom_item.parameters.parameters = bloom_params;

			pp_items.push_back(bloom_item);

			pp_output_image = bloom_item.output_surface;

			camy_info("Successfully loaded Effects_Bloom");
		}

		/*
			Final merging step, done only if HDR is enabled
		*/
		if (effects & Effects_HDR)
		{
			/*
				Preparing tonemap
			*/
			if (!tone_map_ps.load(Shader::Type::Pixel, shader_code::tone_map_ps, sizeof(shader_code::tone_map_ps)))
			{
				camy_error("Error: failed to load \"tone map\" shader");
				dispose();
				return false;
			}

			PostProcessItem tone_map_item;
			tone_map_item.input_surface = pp_output_image; 
			tone_map_item.output_surface = output_image;
			tone_map_item.pixel_shader = &tone_map_ps;

			tone_map_params[0].shader_variable = tone_map_ps.get("bilinear_sampler");
			tone_map_params[0].data = bilinear_sampler;

			tone_map_params[1].shader_variable = tone_map_ps.get("luminance_map");
			tone_map_params[1].data = luminance_mipmaps.back();

			tone_map_item.parameters.parameters = tone_map_params;
			tone_map_item.parameters.num_parameters = 2;

			pp_items.push_back(tone_map_item);
		}

		return true;
	}

	void PostProcessPipeline::dispose()
	{
		pp_items.clear();
		
		/*
			Shared
		*/
		hidden::gpu.safe_dispose(bilinear_sampler);
		hidden::gpu.safe_dispose(additive_blend_state);

		/*
			Effects_HDR
		*/
		hidden::gpu.safe_dispose(offscreen_target);
		hidden::gpu.safe_dispose(luminance_map);

		for (auto& mipmap : luminance_mipmaps)
			hidden::gpu.safe_dispose(mipmap);
		
		luminance_downsample_ps.unload();
		to_luminance_ps.unload();
		tone_map_ps.unload();

		/*
			Effects_Bloom
		*/
		hidden::gpu.safe_dispose(downsampled_bright_scene);
		hidden::gpu.safe_dispose(kawase_blur_extra_rts[0]);
		hidden::gpu.safe_dispose(kawase_blur_extra_rts[1]);

		bright_downsample_ps.unload();
		kawase_blur_ps.unload();
		bloom_ps.unload();
	}
}