struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D	 input_surface;
SamplerState luminance_sampler;

float4 main(PSInput input) : SV_TARGET
{
	// We are letting bilinear filtering take care of downsampling.
	// Some resources:
	// http://www.gamedev.net/topic/651867-question-about-tone-mapping-hdr/#entry5121231
	// http://www.gamedev.net/topic/345577-d3d9-high-dynamic-range-rendering-example/
	// In the demo he samples multiple times to avoid a later on loss of precision ? 
	// I didnt go the Load() approach because it becomes way too complex when dealing with odd / small numbers
	// where if you wrongly sample something you are going to end up with completely wrong results
	float l = input_surface.Sample(luminance_sampler, input.texcoord);

	return float4(l, l, l, 1.f);
}