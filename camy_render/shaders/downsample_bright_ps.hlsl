struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D input_surface;
SamplerState bilinear_sampler; // Bilinear filterring

float4 main(PSInput input) : SV_TARGET
{
	const float3 luminance = float3(0.33f, 0.34f, 0.33f);
	const float brightness_threshold = 0.1f;
	float3 c = input_surface.Sample(bilinear_sampler, input.texcoord).rgb;
	
	float l = dot(c, luminance);
	l = max(0.f, l - brightness_threshold);
	c *= sign(l); // We dont want to scale, we just want to cut the off

	return float4(c, 1.f);
}