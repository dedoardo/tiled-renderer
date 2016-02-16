#pragma once

/*
	Topic: camy_math
		DirectXMath is included only here to avoid bloating headers, wrappers and common 
		definitions are defined here
*/

// Math library
#include <DirectXMath.h>

// C++ STL
#include <cstdint>

namespace camy
{
	/*
		Types are outside the math namespace the reason is that they only reason why functions are inside the math namespace is that
		the function names collide such as load() store() 
	*/

	using float2 = DirectX::XMFLOAT2;
	using float3 = DirectX::XMFLOAT3;
	using float4 = DirectX::XMFLOAT4;

	using uint = std::uint32_t;
	using uint2 = DirectX::XMUINT2;
	using uint3 = DirectX::XMUINT3;
	using uint4 = DirectX::XMUINT4;

	using float4x4 = DirectX::XMFLOAT4X4;

	using vector = DirectX::XMVECTOR;
	using matrix = DirectX::XMMATRIX;

	using p_vector = DirectX::FXMVECTOR;
	using p_matrix = DirectX::FXMMATRIX;

	// Default values
	const float4x4	float4x4_default{ 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
	const float3	float3_default	{ 0.f, 0.f, 0.f };
	const float4	float4_default	{ 0.f, 0.f, 0.f, 1.f }; // 1 by default because i assume is going to be a position vector, 
																		//	anyway W component should not be of intterest when not transforming
	
	const auto default_alignment{ alignof(DirectX::XMVECTOR) };

	namespace math
	{
		// There reason for this function is just to "layer the math api", the main reason is style tho
		// Looks more consistent
		// less stuff to write
		// especially in headers where we can't really use using namespace 
		// last reason is that most of the types are the same as HLSL and we can share headers
		camy_inline vector load(const float4& vec);
		camy_inline vector load(const float3& vec);
		camy_inline matrix load(const float4x4& mat);

		camy_inline void store(float4& vec, p_vector simd_vec);
		camy_inline void store(float3& vec, p_vector simd_vec);
		camy_inline void store(float4x4& mat, p_matrix simd_mat);

		camy_inline vector add(p_vector left, p_vector right);
		camy_inline vector sub(p_vector left, p_vector right);
		camy_inline vector scale(p_vector vec, float factor);

		camy_inline float len_squared3(p_vector vec);

		camy_inline matrix mul(p_matrix left, p_matrix right);
		camy_inline vector mul3(p_vector vec, p_matrix mat);
		camy_inline vector cross(p_vector left, p_vector right);

		camy_inline matrix create_rotation(p_vector axis, float radians);
		camy_inline matrix create_rotation(p_vector roll_pitch_yaw);
		camy_inline matrix create_translation(p_vector position);
		camy_inline matrix create_scaling(float factor);

		camy_inline matrix create_perspective(float fov, float ratio, float near_z, float far_z);
		camy_inline matrix create_orthogonal();
		camy_inline matrix create_look_at(p_vector pos, p_vector at, p_vector up);

		camy_inline matrix transpose(p_matrix mat);
		camy_inline matrix invert(p_matrix mat);

		camy_inline u32 upper_pow2(u32 value);

		const float pi{ DirectX::XM_PI };
	}

	/*
		Topic: Common colors
			I'm using float4 as storage type and not XMVECTORF32 because there will be rarely the need
			to load colors into SIMD registers
	*/
	namespace colors
	{
		// Todo: why is constexpr invalid ? 
		inline const float4 from_packed(const std::uint32_t packed)
		{
			return float4{ static_cast<float>((packed >> 24) & 0xFF) / 255, 
				static_cast<float>((packed >> 16) & 0xFF) / 255, 
				static_cast<float>((packed >> 8) & 0xFF) / 255,
				static_cast<float>(packed & 0xFF) / 255 };
		}

		const float4 white			{ from_packed(0xFFFFFFFF) };
		const float4 black			{ from_packed(0x000000FF) };
		const float4 ivory			{ from_packed(0xFFFFF0FF) };
		const float4 beige			{ from_packed(0xF5F5DCFF) };
		const float4 wheat			{ from_packed(0xF5DeB3FF) };
		const float4 tan			{ from_packed(0xD2B48CFF) };
		const float4 khaki			{ from_packed(0xC3B091FF) };
		const float4 silver			{ from_packed(0xC0C0C0FF) };
		const float4 gray			{ from_packed(0x808080FF) };
		const float4 charcoal		{ from_packed(0x464646FF) };
		const float4 navy_blue		{ from_packed(0x000080FF) };
		const float4 royal_blue		{ from_packed(0x084C9EFF) };
		const float4 medium_blue	{ from_packed(0x0000CDFF) };
		const float4 azure			{ from_packed(0x007FFFFF) };
		const float4 cyan			{ from_packed(0x00FFFFFF) };
		const float4 aquamarine		{ from_packed(0x7FFFD4FF) };
		const float4 teal			{ from_packed(0x008080FF) };
		const float4 forest_green	{ from_packed(0x228B22FF) };
		const float4 olive			{ from_packed(0x808000FF) };
		const float4 chartreuse		{ from_packed(0x7FFF00FF) };
		const float4 lime			{ from_packed(0xBFFF00FF) };
		const float4 golden			{ from_packed(0xFFD700FF) };
		const float4 golden_rod		{ from_packed(0xDAA520FF) };
		const float4 coral			{ from_packed(0xFF7F50FF) };
		const float4 salmon			{ from_packed(0xFA8072FF) };
		const float4 hot_pink		{ from_packed(0xFC9FC9FF) };
		const float4 fuchsia		{ from_packed(0xFF77FFFF) };
		const float4 puce			{ from_packed(0xCC8899FF) };
		const float4 mauve			{ from_packed(0xE0B0FFFF) };
		const float4 lavendere		{ from_packed(0xB57EDCFF) };
		const float4 plum			{ from_packed(0x843179FF) };
		const float4 indigo			{ from_packed(0x4B0082FF) };
		const float4 maroon			{ from_packed(0x800000FF) };
		const float4 crimson		{ from_packed(0xDC143CFF) };
	}
}

#include "camy_math.inl"