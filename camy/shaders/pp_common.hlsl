/*
	Default input coordinates.
*/
struct PSInput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

/*
	Either the specified input surface or the result from the previous postprocess step.
*/
Texture2D input_surface;