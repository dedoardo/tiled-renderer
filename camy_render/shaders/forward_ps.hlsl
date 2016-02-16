#define camy_shaders_enable_environment
#define camy_shaders_enable_material
#include "../include/camy_render/shader_common.hpp"

SamplerComparisonState	comparison_sampler;
SamplerState			default_sampler;
Texture2D				shadow_map;
Texture2D				shadow_map_view;

StructuredBuffer<Light> lights;
StructuredBuffer<uint> light_indices;
StructuredBuffer<uint2> light_grid;

Texture2D color_map;
Texture2D smoothness_map;
Texture2D metalness_map;

struct PSInput
{
	float4 position : SV_POSITION0;
	float3 normal : NORMAL0;
	float2 texcoord : TEXCOORD0;
	float3 tangent : NORMAL1;
	float3 binormal : NORMAL2;
	float4 world_position : TEXCOORD1;
	float4 light_position : TEXCOORD2;
	float4 light_view_position : TEXCOORD3;
};

float F(float angle)
{
	return normal_reflectance + (pow(1 - angle, 5.0)) * (1 - normal_reflectance);
}

float G(float angle)
{
	float kg = 1.01;
	float rkg_2 = 1 / (kg * kg);
	float d1 = (1 - kg) * (1 - kg);
	float d2 = (angle - kg) * (angle - kg);
	float g_num = (1 / d1) - (1 / d2);
	float g_den = (1 / d1) - rkg_2;
	return g_num / g_den;
}

float3 compute_diffuse(float3 N, float3 L, float3 C, float smoothness, float metalness)
{
	float NdotL = dot(N, L);

	float s_3 = smoothness * smoothness * smoothness;
	float t = 0.f; // Not supported yet, set to  0

	float rd = (1 - s_3) * (1 - t);
	float d = (1 - metalness * smoothness);
	return max(0.f, NdotL * d * rd * C);
}

float3 compute_specular(float3 N, float3 L, float3 V, float3 C, float smoothness, float metalness)
{
	float NdotL = dot(N, L);
	float NdotV = dot(N, V);
	float3 H = reflect(L, N);
	float fresnel = F(NdotL);

	float t = 0.f; // Not supported yet, set to 0
	float kj = 0.1f;

	// Interpolating specular color for metallic surfaces
	float3 C1 = float3(1.f, 1.f, 1.f);
	float3 Cs = C1 + metalness * (1 - fresnel) * (C - C1);

	float j = fresnel * G(NdotL) * G(NdotV);

	float s_3 = smoothness * smoothness * smoothness;
	float rd = (1.f - s_3) * (1.f - t);
	float rn = (1.f - t) - rd;
	float rj = min(1, rn + (rn + kj) * j);

	float h = 3.f / (1.f - smoothness);
	float rs = pow(-dot(H, V), h) * rj;

	return max(0.f, Cs * rs);
}

uint get_num_lights(float4 projected_position)
{
	projected_position.xy -= 0.5f;
	uint2 tile = projected_position.xy / float2(camy_tile_size, camy_tile_size);

	uint num_tiles = (uint)ceil(width / camy_tile_size);
	uint index = tile.x + tile.y * num_tiles;

	// Calculating tile index from it
	return light_grid[index].y;
}

float3 evaluate_lights(float4 projected_position, float3 world_position, float3 N, float3 V, float3 C, float smoothness, float metalness)
{
	float3 contribution = float3(0.f, 0.f, 0.f);

	projected_position.xy -= 0.5f;
	uint2 tile = projected_position.xy / float2(camy_tile_size, camy_tile_size);

	uint num_tiles = (uint)ceil(width / camy_tile_size);
	uint index = tile.x + tile.y * num_tiles;

	// Calculating tile index from it
	uint num_lights = light_grid[index].y;
	uint light_offset = light_grid[index].x;

	for (uint i = 0; i < num_lights; ++i)
	{
		Light light = lights[light_indices[light_offset + i]];
		float3 L = normalize(light.position - world_position);
		float d = distance(light.position, world_position);
		
		// max(0, 1 - |x|^2 / r^2)^2
		float ad = abs(d);
		float attenuation = max(0, 1 - (ad * ad) / (light.radius * light.radius));
		attenuation *= attenuation;

		// We use the affect radius as "radius of the light", when d == affect radius 
		// the attenuation Has to be exactly 0 otherwise artifacts would show up
		// https://imdoingitwrong.wordpress.com/2011/02/10/improved-light-attenuation/
		// As shown in the above paper bias + scaling doesn't really bring us to a perfect 0,
		// I might improve on this in the future that's why im keeping it here as a reference
		// actually im biasing and scaling
		//float attenuation = light.intensity / ((d / light.radius) * (d / light.radius));
		//attenuation -= 0.1;
		//attenuation *= (1.f / (1 - 0.1));
//
		contribution += (compute_diffuse(N, L, C, smoothness, metalness) + compute_specular(N, L, V, C, smoothness, metalness)) * light.color * attenuation;
	}	

	return contribution;
}

float4 main(PSInput input) : SV_TARGET
{
	input.world_position.xyz /= input.world_position.w;

	float3 V = normalize(eye_position - input.world_position.xyz);
	float3 N = normalize(input.normal);
	float3 L = normalize(-light_direction);

	// Calculating shadow factor
	float shadow_factor = 1.f;
	
	input.light_position.xyz /= input.light_position.w;

	// NDC => Texture space
	input.light_position.x = input.light_position.x / 2 + 0.5;
	input.light_position.y = input.light_position.y / -2 + 0.5;

	// Todo : Automatic bias WAY bigger than normal biasing because we are in view space and 0.001 is literally nothing ( could be slightly less tho )
	input.light_position.z -= 0.005;
	 
	float sum = 0;
	float x, y;
	for (y = -1.5; y <= 1.5; y += 1.0)
	{
		for (x = -1.5; x <= 1.5; x += 1.0)
		{
			// The shadow map is in view space that'
			float2 offset = float2(x / 4096, y / 4096);
			float depth_sample = shadow_map.SampleCmpLevelZero(comparison_sampler, input.light_position.xy + offset, input.light_position.z);
			sum += depth_sample;
		}
	}

	shadow_factor = sum / 16;

	float3 final_color = float3(0.f, 0.f, 0.f);

	float3 surface_color = base_color;
	float  surface_smoothness = smoothness;
	float  surface_metalness = metalness;

	if (render_feature_set & RenderFeatureSet_ColorMap)
		surface_color = color_map.Sample(default_sampler, input.texcoord.xy).rgb;

	if (render_feature_set & RenderFeatureSet_SmoothnessMap)
		surface_smoothness = smoothness_map.Sample(default_sampler, input.texcoord.xy).r / 2;

	// Adding some ambient
	final_color += ambient_factor * surface_color;

	// Calculating contribution from directional light
	float3 sun_color = float3(1.f, 1.f, 1.f);
	final_color += (compute_diffuse(N, L, surface_color, surface_smoothness, surface_metalness) + compute_specular(N, L, V, surface_color, surface_smoothness, surface_metalness)) * sun_color * shadow_factor * intensity;

	// Currently not a full BRDF / SSBRDF is implement, 
	// simply an exponential falloff based on the base_color
	// In the future it will be implement as a postprocessing effect as screenspace:
	// http://www.iryoku.com/publications
	// No texture space diffusion
	if (render_feature_set & RenderFeatureSet_Translucent)
	{
	//	float light_vs_depth = shadow_map_view.Sample(default_sampler, input.light_position.xy).r;
	//	float light_td = abs(input.light_view_position.z - light_vs_depth);
	//	float tfactor = exp((-light_td + 1.f) * 0.9f);
	//	final_color = saturate(tfactor * base_color);
	}

	// Calculating contribution from other lights ( properly culled)
	final_color += evaluate_lights(input.position, input.world_position.xyz, N, V, surface_color, surface_smoothness, surface_metalness);

	// Uncomment for light culling "debug"
	//final_color *= 0.5f;
	//final_color += get_num_lights(input.position) * float3(0.2f, 0.f, 0.f);
	
	return float4(final_color, 1.f);
}