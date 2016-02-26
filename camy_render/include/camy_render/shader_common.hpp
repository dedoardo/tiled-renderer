#if defined(camy_compile_cpp)
#pragma once

#include <camy/base.hpp>
#include <camy/math.hpp>

#define cbuffer struct
#define uint	u32

#pragma pack(push, 1)

namespace camy
{
	namespace shaders
	{
#if defined(camy_compile_cpp)
		/*
			Names for all the possible resources to be bound to the shaders
		*/
		namespace names
		{
			static const char next_light_index[]{ "next_light_index" };
			static const char light_indices[]{ "light_indices" };
			static const char light_grid[]{ "light_grid" };
			static const char lights[]{ "lights" };
			static const char depth_map[]{ "depth_map" };

			static const char shadow_map[]{ "shadow_map" };
			static const char shadow_map_view[]{ "shadow_map_view" };

			static const char default_sampler[]{ "default_sampler" };
			static const char comparison_sampler[]{ "comparison_sampler" };

			static const char color_map[]{ "color_map" };
			static const char smoothness_map[]{ "smoothness_map" };
			static const char metalness_map[]{ "metalness_map" };
		}
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

#if defined(camy_shaders_enable_luminance_downsample_args) || defined(camy_compile_cpp)
		cbuffer LuminanceDownsampleArgs
		{
#if defined(camy_compile_cpp)
			static const char* name;
#endif
			float2 texel_size;
		};
#endif


#if defined(camy_shaders_enable_kawase_blur_args) || defined(camy_compile_cpp)
		cbuffer KawaseBlurArgs
		{

#if defined(camy_compile_cpp)
			static const char* name;
#endif
			float2 texel_size;
			float2 _ImageInfoPadding1;
			uint  iteration;
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
	}
}

#endif