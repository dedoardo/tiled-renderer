struct PosOnlyOutput
{
	float4 position : SV_POSITION0;
	float4 view_position : TEXCOORD0;
};

float4 main(PosOnlyOutput input) : SV_TARGET
{
	return input.view_position.z;
}