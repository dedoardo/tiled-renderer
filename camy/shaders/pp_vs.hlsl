struct VSOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

//http://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau
VSOutput main(uint vertex_id : SV_VERTEXID)
{
	VSOutput output;

	output.position.x = (float)(vertex_id / 2) * 4.f - 1.f;
	output.position.y = (float)(vertex_id % 2) * 4.f - 1.f;
	output.position.z = 1.f;
	output.position.w = 1.f;

	output.texcoord.x = (float)(vertex_id / 2) * 2.f;
	output.texcoord.y = 1.f - (float)(vertex_id % 2) * 2.f;

	return output;
}