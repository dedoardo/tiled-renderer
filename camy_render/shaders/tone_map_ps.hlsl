struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

Texture2D	 color_map : register(t0);	// Either output of previous postprocessing effects or input image
Texture2D	 luminance_map; // 1x1 luminance map
SamplerState bilinear_sampler;

float3 RGB2Yxy(float3 rgb)
{
	const float3x3 RGB2XYZ = { 0.5141364, 0.3238786, 0.16036376, 0.265068, 0.67023428, 0.06409157, 0.0241188, 0.1228178, 0.84442666 };
	float3 XYZ = mul(RGB2XYZ, rgb);

	float3 Yxy;
	Yxy.r = XYZ.g;
	Yxy.g = XYZ.r / (XYZ.r + XYZ.g + XYZ.b);
	Yxy.b = XYZ.g / (XYZ.r + XYZ.g + XYZ.b);  // Could dot

	return Yxy;
}

float3 Yxy2RGB(float3 Yxy)
{
	const float3x3 XYZ2RGB = { 2.5651,-1.1665,-0.3986, -1.0217, 1.9777, 0.0439, 0.0753, -0.2543, 1.1892 };
	float3 XYZ;
	XYZ.r = Yxy.r * Yxy.g / Yxy.b;
	XYZ.g = Yxy.r;
	XYZ.b = Yxy.r * (1 - Yxy.g - Yxy.b) / Yxy.b;
	
	return mul(XYZ2RGB, XYZ);
}

float4 main(PSInput input) : SV_TARGET
{
	// Sampling luminance
	float global_luminance = exp(luminance_map.Sample(bilinear_sampler, float2(0.5f, 0.5f)).r); // is Load(0,0,0) faster ?

	// Sampling color
	float3 color = color_map.Sample(bilinear_sampler, input.texcoord).rgb;

	float3 Yxy = RGB2Yxy(color);

	// Moving color to luminance
	const float exposure = 0.18; // Make this parameter and resolution uniforms
	Yxy.r = (exposure / global_luminance) * Yxy.r;

	// Down to 0-1 range ( easy equation here ) 
	Yxy.r = Yxy.r / (1 + Yxy.r);

	float3 ret = Yxy2RGB(Yxy);

	return float4(ret, 1.f);
}