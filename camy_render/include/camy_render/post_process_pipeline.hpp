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
		PostProcessPipeline() = default;
		~PostProcessPipeline();

		
		// Loads all the required resources and fills in pp_items
		bool load(Surface* hdr_scene, Surface* output_rt);
		
		// Manually disposes everything
		void dispose();

		// Postprocessing items
		std::vector<PostProcessItem> pp_items;

		// Luminance mipmaps and map
		std::vector<Surface*> luminance_mipmaps;
		Surface* downsampled_bright_scene;

		Surface* luminance_map;

		Sampler* default_sampler;
		PipelineParameter downsample_luminance_sampler_parameter;
		PipelineParameter to_luminance_sampler_parameter;

		// From HDR rendered scene to luminance map, same resolution
		Shader to_luminance_ps;

		// Downsamples luminance values using some operator
		Shader luminance_downsample_ps;

		// Downsamples the color taking into account brightness and threshold for the bloom filter
		Shader bright_downsample_ps;

		// Blurring shaders, currently fixed kernel
		Shader blur_vertical_ps;
		Shader blur_horizontal_ps;

		// Combines the output of the bloom pass ( that is the color ) that will then 
		// be tone-mapped accordingly to the world luminance computed and stored in the
		// 1x1 texture
		Shader tone_map_ps;
	};
}