#define camy_shaders_enable_per_frame_light
#define camy_shaders_enable_per_object
#include "../include/camy_render/shader_common.hpp"

struct VSInput
{
	float3 position : POSITION0;
	
	float3 normal : NORMAL0;
	float2 texcoord : TEXCOORD0;
	float3 tangent : NORMAL1;
	float3 binormal : NORMAL2;
};

struct PSInput
{
	float4 position : SV_POSITION0;
	float3 normal : NORMAL0;
	float2 texcoord : TEXCOORD0;
	float3 tangent : NORMAL1;
	float3 binormal : NORMAL2;
	float4 world_position : TEXCOORD1;
	float4 light_position : TEXCOORD2;
	float4 light_view_position : TEXCOORD3;
};

PSInput main(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1.f), world);
	output.position = mul(output.position, view_projection);
	
	output.normal = mul( input.normal, (float3x3)world);
	output.normal = normalize(output.normal);

	output.texcoord = input.texcoord;

	output.tangent = mul(input.tangent, (float3x3)world);
	output.tangent = normalize(output.tangent);

	output.binormal = mul(input.binormal, (float3x3)world);
	output.binormal = normalize(output.binormal);

	// Todo: rewrite, why using only xyz shouldn't we divide by w
	output.world_position = mul(float4(input.position, 1.f), world);

	output.light_position = mul(float4(input.position, 1.f), world);
	output.light_position = mul(output.light_position, view_projection_light);

	output.light_view_position = mul(float4(input.position, 1.f), world);
	output.light_view_position = mul(output.light_view_position, view_light);

	return output;
}