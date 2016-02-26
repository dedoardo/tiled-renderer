struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D input_surface;

float4 main(PSInput input) : SV_TARGET
{
	// Parametise
	int3 loc = uint3(input.texcoord * int2(1920, 1080), 0);

	return float4(input_surface.Load(loc).xyz, 1.f);
}