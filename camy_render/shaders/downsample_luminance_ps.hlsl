struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D	 input_surface;
SamplerState luminance_sampler;

float4 main(PSInput input) : SV_TARGET
{
	// We are letting bilinear filtering taking care of doing the downsampling
	float l = input_surface.Sample(luminance_sampler, input.texcoord);

	return float4(l, l, l, 1.f);
}