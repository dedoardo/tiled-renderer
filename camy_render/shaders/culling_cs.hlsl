#define camy_shaders_enable_culling_dispatch_args
#include "../include/camy_render/shader_common.hpp"

// IN
Texture2D depth_map;
StructuredBuffer<Light>   lights;

// OUT
// The first is a counter, it should be a global variable shared between thread groups
RWStructuredBuffer<uint> next_light_index; // Just 1 element that is the next global index
RWStructuredBuffer<uint> light_indices;
RWStructuredBuffer<uint2> light_grid;

// Atomic operations on floats are not supported
groupshared uint current_min_depth;
groupshared uint current_max_depth;

groupshared uint index_offset_out;
groupshared uint next_index;
groupshared uint light_list[camy_max_num_lights_per_tile];

struct CSInput
{
	uint3 group_id : SV_GroupID;
	uint3 group_thread_id : SV_GroupThreadID;
	uint3 dispatch_thread_id : SV_DispatchThreadID; // global thread index
	uint  group_index : SV_GroupIndex;
};

[numthreads(camy_tile_size, camy_tile_size, 1)]
void main(CSInput input)
{
	float depth = depth_map.Load(int3(input.dispatch_thread_id.xy, 0)).r;

	// Not casting just interpreting
	uint udepth = asuint(depth);

	// We need to reset min & max, just one thread does it, doesn't matter which one
	if (input.group_thread_id.x == 0 &&
		input.group_thread_id.y == 0)
	{
		current_min_depth = 0x7F7FFFFF;
		// first 7 is because of the sign 1 bit sign 8 bit exponent 23 bit fraction ( standard IEEE single precision )
		// second 7 is because of the offset binary representation where out of 8 bits 127 is the maximum exponent thus 0111 in two's binary complement 
		/// https://en.wikipedia.org/wiki/Single-precision_floating-point_format ( see exponent encoding )
		// https://en.wikipedia.org/wiki/Offset_binary
		current_max_depth = 0;
		next_index = 0;
	}

	// Let's wait for all threads
	GroupMemoryBarrierWithGroupSync();

	// Let's avoid counting skybox

	if (depth <= far && depth >= near)
	{
		InterlockedMin(current_min_depth, udepth);
		InterlockedMax(current_max_depth, udepth);
	}

	// Again let's wait for all the threads
	GroupMemoryBarrierWithGroupSync();

	float min_depth = asfloat(current_min_depth);
	float max_depth = asfloat(current_max_depth);

	// Computing frustum, frustum is shared on a tile basis, but the math here is quite simple and code-wise we avoid running a shader at the beginning 
	// and passing parameters. 
	/*
		We are using the clip space approach, it could be done using a view approach and it's easier to understand since it directly operates in
		view space. The following approach i find it very interesting though and trying to figure it out is kinda hard for me but i 'll do my best.
		Note: Following will be notes that i took while trying to understand the construction of the asymmetric frustums for the single tile,
		resources can be found :
		[1]https://software.intel.com/en-us/articles/deferred-rendering-for-current-and-future-rendering-pipelines
		[2]http://developer.amd.com/tools-and-sdks/graphics-development/amd-radeon-sdk/#downloadsamples
		[3]http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
		[4]http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-extracting-the-planes/

		First off, frustum extraction is done using a newly build projection matrix, it is "built" on top of the current one
		that we use for some of its parameters. Once the matrix has been created planes are extracted following [3] or [4] ( that is the clip space approach ).
		I find [3] more clear than [4].
		We only extract top/bottom and left/right planes, near and far are ignored since z manual culling will be done later after having the scene depth buffer. Doing
		it with the full-blown matrix would be wrong since lights that are not seen on screen would be seen aswell while they should be occluded ( TODO: example )
		That's why we only care about the 1, 2, 4 cols. This is the classical perspective projection matrix:
		C0 2n / (r-l)    0        r+l/r-l    0
		C1 0            2n/(t-b)  t+b/t-b    0
		C2 We don't care
		C3 0             0           1       0
		We are using 1 because we are in a left-handed coordinate system with +Z pointing away from the user ( in front )

		What we need to do i+n order to build the projection matrix for the "single" tile is correctly scale the components
	
		Note: The division by 2 is because the scale/bias is computed on a [0,1] scale and not a [-1, 1] left is 0 and right is framebuffer width, same for 0, height( I think )
		Let's work out the factors 
		(r-l) needs to be scaled to the single tile's size, thus we multiply 2n / (r-l) * tile_scale, where
		tile_scale = viewport_dimensions / (2 * tile_size), the 2 as said above is because we are working in the 0, 1 range, not -1, 1
		
		now the second term is (r+l) / (r-l)
	*/
	float2 tile_scale = float2(width, height) * rcp(float(2 * camy_tile_size));
	float2 tile_bias = tile_scale - float2(input.group_id.xy); // Todo: Haven't entirely figured this out

	// Rebuilding projection matrix with properly scaled components
	float4 col0 = float4(projection._11 * tile_scale.x, 0.f, tile_bias.x, 0.f);
	float4 col1 = float4(0.f, -projection._22 * tile_scale.y, tile_bias.y, 0.f);
	float4 col3 = float4(0.f, 0.f, 1.f, 0.f);

	// See [3], not computing min/max planes
	float4 planes[4];
	planes[0] = col3 - col0; // Right
	planes[1] = col3 + col0; // Left
	planes[2] = col3 - col1; // Top
	planes[3] = col3 + col1; // Bottom

	// Normalizing all the 4 components, W is distance it is normalized but does not
	// take part into the length calculation
	[unroll] // Don't you dare not unroll it, just making sure
	for (uint i = 0; i < 4; ++i)
		planes[i] *= rcp(length(planes[i].xyz));

	// Time to loop through the lights and cull them based on distance & plane frustums,
	// Compute shader runs once for each texel, need to calculate the index, we do this using 
	// the index of the thread relative to the working group
	uint thread_start_index = input.group_thread_id.x + input.group_thread_id.y * camy_tile_size;
	for (i = thread_start_index; i < num_lights; i += (camy_tile_size * camy_tile_size))
	{
		Light light = lights[i];

		bool is_inside = true;

		// We need to transform the light's position from world to view space ( *NOT* view_projection ),
		// radius remains the same since no scaling is occurring, assuming position is already is world coordinates
		// Rotation doesn't make sense for point lights and scaling is done via its radius
		float3 light_position = mul(float4(light.position, 1.f), view).xyz; // Not going into projective space

		float affect_radius = light.radius;

		// Checking depth
		if (light_position.z + affect_radius <= min_depth ||
			light_position.z - affect_radius >= max_depth)
			is_inside = false;
			

		// Checking frustums
		for (uint j = 0; j < 4; ++j)
		{
			float d = dot(planes[j], float4(light_position, 1.f));
			// Todo: read feature lists and see difference between radius & attenuation end
			if (d < -affect_radius)
				is_inside = false;
		}

		// If light is inside, time to add it
		if (is_inside)
		{
			// Reserving index and writing light
			uint index;
			InterlockedAdd(next_index, 1, index);
			light_list[index] = i; // Just writing index
		}
	}

	// Waiting for all threads
	GroupMemoryBarrierWithGroupSync();

	// Now time for each group ( first thread of each group ) to write its results 
	// to the global list
	if (input.group_thread_id.x == 0 &&
		input.group_thread_id.y == 0)
	{
		InterlockedAdd(next_light_index[0], next_index, index_offset_out);
		
		// Im not currently using a texture2d, just flattening the index, each thread group is a tile
		uint grid_index = input.group_id.x + input.group_id.y * num_tiles.x;
		light_grid[grid_index] = uint2(index_offset_out, next_index);
	}

	// Waiting for all threads
	GroupMemoryBarrierWithGroupSync();

	// Let's take advantage of the single thread per pixel again, this time we just copy the light indices in the final light indices
	for (i = thread_start_index; i < next_index; i += (camy_tile_size * camy_tile_size))
		light_indices[index_offset_out + i] = light_list[i];
}