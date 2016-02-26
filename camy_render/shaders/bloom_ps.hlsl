struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D	 blurred_image;
Texture2D	 original_image;

SamplerState bilinear_sampler;

float4 main(PSInput input) : SV_TARGET
{
	// Blendstate based additive blending
	return blurred_image.Sample(bilinear_sampler, input.texcoord.xy);
}