struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D input_surface;
SamplerState surface_sampler; // Bilinear filtering

float4 main(PSInput input) : SV_TARGET
{
	// Parametise
	const float delta = 0.0001f; // Accounting for black pixels
	const float3 luminance = float3(0.33f, 0.34f, 0.33f);
	float3 c = input_surface.Sample(surface_sampler, input.texcoord);
	float l = log(dot(c, luminance) + delta);

	return float4(l.r, l.r, l.r, 1.f);
}