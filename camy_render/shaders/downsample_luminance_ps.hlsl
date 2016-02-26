#define camy_shaders_enable_luminance_downsample_args
#include "../include/camy_render/shader_common.hpp"

struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D	 input_surface;
SamplerState point_sampler;

float4 main(PSInput input) : SV_TARGET
{
	input.texcoord -= (texel_size / 2);

	float average_luminance = 0.f;
	average_luminance += input_surface.Sample(point_sampler, input.texcoord);
	average_luminance += input_surface.Sample(point_sampler, input.texcoord + float2(texel_size.x, 0.f));
	average_luminance += input_surface.Sample(point_sampler, input.texcoord + texel_size);
	average_luminance += input_surface.Sample(point_sampler, input.texcoord + float2(0.f, texel_size.y));
	average_luminance *= 0.25f;

	return float4(average_luminance.xxx, 1.f);
}