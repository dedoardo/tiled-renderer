#define camy_shaders_enable_per_frame
#define camy_shaders_enable_per_object
#include "../include/camy_render/shader_common.hpp"

struct PosOnlyInput
{
	float3 position : POSITION0;
};

struct PosOnlyOutput
{
	float4 position : SV_POSITION0;
};

PosOnlyOutput main(PosOnlyInput input)
{
	PosOnlyOutput output;
	output.position = mul(float4(input.position, 1.f), world);
	output.position = mul(output.position, view_projection);

	return output;
}