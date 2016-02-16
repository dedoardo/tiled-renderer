#if defined(camy_compile_cpp)
#pragma once

#include <camy/camy_base.hpp>
#include <camy/camy_math.hpp>

#define cbuffer struct
#define uint	u32
#define float2 camy::float2

#pragma pack(push, 1)

namespace camy
{
	namespace shaders
	{
#if defined(camy_compile_cpp)
		// Todo: need to rework the in/out naming when interfaceing with C++
		static const char next_light_index_name[]{ "next_light_index" };
		static const char light_indices_name[]{ "light_indices" };
		static const char light_grid_name[]{ "light_grid" };
		static const char lights_name[]{ "lights" };
		static const char depth_map_name[]{ "depth_map" };

		static const char shadow_map_name[]{ "shadow_map" };
		static const char shadow_map_view_name[]{ "shadow_map_view" };

		static const char default_sampler_name[]{ "default_sampler" };
		static const char comparison_sampler_name[]{ "comparison_sampler" };

		static const char color_map_name[]{ "color_map" };
		static const char smoothness_map_name[]{ "smoothness_map" };
		static const char metalness_map_name[]{ "metalness_map" };
#endif

#endif
		static const uint RenderFeatureSet_Default			  = 0;
		static const uint RenderFeatureSet_Translucent		  = 1 << 1;
		static const uint RenderFeatureSet_ColorMap			  = 1 << 2;
		static const uint RenderFeatureSet_SmoothnessMap	  = 1 << 3;
		static const uint RenderFeatureSet_MetalnessMap		  = 1 << 4;
		static const uint RenderFeatureSet_Emissive			  = 1 << 5;
		static const uint RenderFeatureSet_BumpMapping		  = 1 << 6;
		static const uint RenderFeatureSet_AmbientOcclusion   = 1 << 7;

#define camy_tile_size 16
#define camy_average_num_lights 10
#define camy_max_num_lights_per_tile 10

#if defined(camy_shaders_enable_per_frame )|| defined(camy_compile_cpp)
		cbuffer PerFrame
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float4x4 view_projection;
		};
#endif

#if defined(camy_shaders_enable_per_frame_non_mul) || defined(camy_compile_cpp)
		cbuffer PerFrameView
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float4x4 view;
			float4x4 projection;
		};
#endif

#if defined(camy_shaders_enable_per_frame_light )|| defined(camy_compile_cpp)
		cbuffer PerFrameLight
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float4x4 view_projection;
			float4x4 view_projection_light;
			float4x4 view_light;
		};
#endif

#if defined(camy_shaders_enable_per_object )|| defined(camy_compile_cpp)
		cbuffer PerObject
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float4x4 world;
		};
#endif

#if defined(camy_shaders_enable_per_frame_and_object )|| defined(camy_compile_cpp)
		cbuffer PerFrameAndObject
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float4x4 view_projection;
			float4x4 world;
		};
#endif 

#if defined(camy_shaders_enable_material )|| defined(camy_compile_cpp)
		cbuffer Material
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float3 base_color;
			float  smoothness;
			float  metalness;
			float  normal_reflectance; // air => material
			float2 _MaterialPadding1;
			uint   render_feature_set;
		};
#endif 

#if defined(camy_shaders_enable_environment )|| defined(camy_compile_cpp)
		cbuffer Environment
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float3 eye_position;
			float  ambient_factor; // Will be eventually removed when going to irradiance maps ( someday somehow )
			float3 light_direction;
			float  intensity;
			float3 light_color;
			float  _EnvironMentPadding1;
			float  width;
			float  height;
			float  near;
			float  far;
		};
#endif

#if defined(camy_shaders_enable_culling_dispatch_args) || defined(camy_compile_cpp)
		cbuffer CullingDispatchArgs
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif

			float4x4 projection;
			float4x4 view;
			float    width;
			float	 height;
			float	 near;
			float    far;
			uint3	 num_tiles;
			uint	 num_lights;
		};
#endif

#if defined(camy_shaders_enable_image_info) || defined(camy_compile_cpp)
		cbuffer ImageInfo
		{

#if defined(camy_compile_cpp)
			static const char* name;
#endif
			uint width;
			uint height;
			uint2 _ImageInfoPadding1;
			float2 offsets[4];
		};
#endif

		/*
			Currently we only support point lights
		*/
		struct Light
		{
			float3 position;
			float  radius;  // Physical radius of the light, distance from point to center,
							// attenuation is calculated based on some prefedined threshold
			float3 color;
			float  intensity;
		};

#if defined(camy_compile_cpp)

#pragma pack(pop)

#undef cbuffer
#undef uint
#undef float2
	}
}

#endif