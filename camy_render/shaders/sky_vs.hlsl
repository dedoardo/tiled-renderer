#define camy_shaders_enable_per_frame_and_object
#include "../include/camy_render/shader_common.hpp"

struct VSInput
{
	float3 position : POSITION0;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float3 local_position : TEXCOORD0;
};

PSInput main(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.f), world);
	output.position = mul(output.position, view_projection);

	output.local_position = input.position;

	return output;
}