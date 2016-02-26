#pragma once

// camy
#include <camy/shader.hpp>
#include <camy/common_structs.hpp>
#include <camy/allocators/paged_linear_allocator.hpp>

// render
#include "shader_common.hpp"

namespace camy
{
	/*
		Container for all shaders and resources needed for the post processing,
		here for simplicity and in order not to clutter the renderer code
	*/
	struct PostProcessPipeline
	{
		/*
			Temporarily hardcoded
		*/
		static const Surface::Format hdr_format{ Surface::Format::RGBA16Float };
		static const u32 kawase_blur_iterations{ 5 };
		static const u32 kawase_blur_kernels[];

		/*
			Enum: Effects
				Effects currently supported by the pipeline
		*/
		enum Effects : u32
		{
			Effects_None  = 0,
			Effects_HDR   = 1 << 0,   // + Tonemapping
			Effects_Bloom = 1 << 1, //
			
			// Yet to implement, here for reference
			Effects_Vignette		= 1 << 2,
			Effects_MotionBlur		= 1 << 3,
			Effects_LensFlare		= 1 << 4,
			Effects_LightScattering = 1 << 5
		};

		PostProcessPipeline() = default;
		~PostProcessPipeline();

		/*
			Function: load
				Creates all the required resources needed for rendering based off the
				number of effects.
		*/
		bool load(Surface* input_image, Surface* output_image, u32 effects);
		
		/*
			Function: dispose
				Manually releases all the resources, automatically called upon destruction
		*/
		void dispose();


		u32 effects{ Effects_None };

		/*
			Postprocessing items
		*/
		std::vector<PostProcessItem> pp_items;

		/*
			Shared
		*/
		Sampler* bilinear_sampler{ nullptr };
		Sampler* point_sampler{ nullptr };
		BlendState* additive_blend_state{ nullptr };

		/*
			Effects_HDR
		*/

		Surface* offscreen_target{ nullptr };

		Surface* luminance_map{ nullptr };
		std::vector<Surface*> luminance_mipmaps;

		Shader luminance_downsample_ps;
		PipelineParameter* luminance_downsample_params{ nullptr }; // Args + Sampler
		shaders::LuminanceDownsampleArgs* luminance_downsample_args{ nullptr };

		Shader to_luminance_ps;
		PipelineParameter to_luminance_sampler_parameter;

		Shader tone_map_ps;
		PipelineParameter tone_map_params[2]; // Average luminance + sampler

		/*
			Effects_Bloom
		*/
		Surface* downsampled_bright_scene{ nullptr };
		Shader bright_downsample_ps;
		PipelineParameter bright_downsample_params;

		Surface* kawase_blur_extra_rts[2]{ nullptr, nullptr };
		Shader kawase_blur_ps;
		PipelineParameter kawase_blur_params[2 * kawase_blur_iterations];						  // Sampler + CBuffer(texel_size, iter)
		shaders::KawaseBlurArgs kawase_blur_args[kawase_blur_iterations]; // One arg per iteration

		Shader bloom_ps;
		PipelineParameter bloom_params[2]; // Input Surface + sampler
	};
}