#pragma once

// camy
#include <camy/core/base.hpp>

/*
	Here the user-friendly interface is provided that uses simd internally
	but is not exposed to the user ( especially useful for simple operation ).
	For more complex or where you want real control on simd types see simd.hpp
	simd.hpp uses the types here as storage types, calling for instance
	dot on a float3 is the same as calling:
	```
	store(dest, dot(load(left), load(right)));
	```

	All member functions return a new instance to ensure a more fluid API
	float4 is homogeneous float3 ( W defaults to 1 )
	except for normalize that modifies the current one and returns a new one too
*/
namespace camy
{
	constexpr float kPi = 3.141592654f;
	constexpr float k2Pi = kPi * 2.f;
	constexpr float k4Pi = kPi * 4.f;
	constexpr float k1OverPi = 1.f / kPi;
	constexpr float k1Over2Pi = 1.f / k2Pi;
	constexpr float kPiOver2 = kPi / 2.f;
	constexpr float kPiOver4 = kPi / 4.f;

	// Not really used, just for storing ( no ops tho )
	using uint = uint32;
	struct uint2 { uint32 x, y; };
	struct uint3 { uint32 x, y, z; };
	struct uint4 { uint32 x, y, z, w; };

	using sint = sint32;
	struct sint2 { sint32 x, y; };
	struct sint3 { sint32 x, y, z; };
	struct sint4 { sint32 x, y, z, w; };

	// <> Float2
	struct float2
	{
		float2() : x(0.f), y(0.f) { }
		float2(float x, float y) : x(x), y(y) { }

		float2& operator+=(const float2& right);
		float2& operator-=(const float2& right);
		float2& operator*=(const float factor);
		float2& operator/=(const float factor);

		float len()const;
		float len_squared()const;
		float dot(float2& right)const;
		float2& normalize();

		float x, y;
	};

	bool   operator==(const float2& left, const float2& right);
	float2 operator-(const float2& right);
	float2 operator+(const float2& left, const float2& right);
	float2 operator-(const float2& left, const float2& right);
	float2 operator*(const float2& left, const float factor);
	float2 operator/(const float2& left, const float factor);

	// Forward decls
	struct float4x4;

	// <> Float3
	struct float3
	{
		float3() : x(0.f), y(0.f), z(0.f) { }
		float3(float x, float y, float z) : x(x), y(y), z(z) { }

		float3& operator+=(const float3& right);
		float3& operator-=(const float3& right);
		float3& operator*=(const float right);
		float3& operator/=(const float right);

		float len()const;
		float len_squared()const;
		float dot(float3& right)const;
		float3 cross(float3& right)const;
		float3& normalize();
		float3& transform(const float4x4& matrix);

		float x, y, z;
	};

	float3 operator-(const float3& right);
	float3 operator+(const float3& left, const float3& right);
	float3 operator-(const float3& left, const float3& right);
	float3 operator*(const float3& left, const float factor);
	float3 operator/(const float3& left, const float right);

	// <> Float4 
	struct float4
	{
		float4() : x(0.f), y(0.f), z(0.f), w(1.f) { }
		float4(float x, float y, float z) : x(x), y(y), z(z), w(1.f) { } 
		float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }

		float4& operator+=(const float4& right);
		float4& operator-=(const float4& right);
		float4& operator*=(const float right);
		float4& operator/=(const float right);

		float len3()const;
		float len_squared3()const;
		float dot3(float4& right)const;
		float4 cross3(float4& right)const; // This is a cross3
		float4& normalize3();
		float4& transform(const float4x4& matrix);

		float x, y, z, w;
	};

	float4 operator-(const float4& right);
	float4 operator+(const float4& left, const float4& right);
	float4 operator-(const float4& left, const float4& right);
	float4 operator*(const float4& left, const float factor);
	float4 operator/(const float4& left, const float right);

	// <> Float4x4
	// Row-major 4x4 matrix
	// defaults to identity
	struct float4x4
	{
		float4 rows[4];

		float4x4() : rows{  { 1.f, 0.f, 0.f, 0.f },
							{ 0.f, 1.f, 0.f, 0.f },
							{ 0.f, 0.f, 1.f, 0.f },
							{ 0.f, 0.f, 0.f, 1.f }
							} { }

		float4x4(float r00, float r01, float r02, float r03,
			float r10, float r11, float r12, float r13,
			float r20, float r21, float r22, float r23,
			float r30, float r31, float r32, float r33) :

			rows{ { r00, r01, r02, r03 },
					{ r10, r11, r12, r13},
					{ r20, r21, r22, r23},
					{ r30, r31, r32, r33}
					} { }

		float4x4& transpose();
			
		/*
			Evaluates what the current math library uses and what the target
			shader language expects ( preprocessor defs ) and eventually 
			transposes the matrix 
		*/
		float4x4& to_shader_order();
	};

    float4x4 operator*(const float4x4& left, const float4x4& right);
}