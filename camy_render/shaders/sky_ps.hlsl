// Assuming max height

struct PSInput
{
	float4 position : SV_POSITION;
	float3 local_position : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 gradient = float3(1.f, 1.f, 1.f);
	gradient *= (input.local_position + 0.5f);

	return float4(gradient, 1.f);
}