#define camy_shaders_enable_per_frame_non_mul
#define camy_shaders_enable_per_object
#include "../include/camy_render/shader_common.hpp"

struct PosOnlyInput
{
	float3 position : POSITION0;

	float3 normal : NORMAL0;
	float2 texcoord : TEXCOORD0;
	float3 tangent : NORMAL1;
	float3 binormal : NORMAL2;
};

struct PosOnlyOutput
{
	float4 position : SV_POSITION0;
	float4 view_position : TEXCOORD0;
};

PosOnlyOutput main(PosOnlyInput input)
{
	PosOnlyOutput output;

	// This grow factor avoid a similar effect of depth peeling when rendering translucent objects
	const float grow = 0.05f;
	float3 p = input.position + input.normal * grow;

	output.position = mul(float4(p, 1.f), world);
	output.position = mul(output.position, view);
	output.position = mul(output.position, projection);

	output.view_position = mul(float4(p, 1.f), world);
	output.view_position = mul(output.view_position, view);

	return output;
}