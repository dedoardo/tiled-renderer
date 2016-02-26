#define camy_shaders_enable_kawase_blur_args
#include "../include/camy_render/shader_common.hpp"

struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D	 input_surface;
SamplerState bilinear_sampler;

// Kawase blur
// https://software.intel.com/en-us/blogs/2014/07/15/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms
// Frame Buffer Postprocessing Effects in DOUBLE-S.T.E.A.L (Wreckless)
float4 main(PSInput input) : SV_TARGET
{
	float2 delta = (0.5 + iteration) * texel_size;

	float3 acc = float3(0.f, 0.f, 0.f);
	acc += input_surface.Sample(bilinear_sampler, input.texcoord + float2(delta.x, delta.y));
	acc += input_surface.Sample(bilinear_sampler, input.texcoord + float2(-delta.x, delta.y));
	acc += input_surface.Sample(bilinear_sampler, input.texcoord + float2(delta.x, - delta.y));
	acc += input_surface.Sample(bilinear_sampler, input.texcoord + float2(-delta.x, -delta.y));
	acc *= 0.25;

	return float4(acc, 1.f);
}