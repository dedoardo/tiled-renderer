/* math.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core.hpp>

#if CAMY_ENABLE_LOGGING > 0
#include <ostream>
#endif

namespace camy
{
    constexpr float PI = 3.141592654f;
    constexpr float PI2 = PI * 2.f;
    constexpr float PI4 = PI * 4.f;

    // Not really used, just for storing ( no ops tho )
    using uint = uint32;
    struct uint2
    {
        uint32 x, y;
    };
    struct uint3
    {
        uint32 x, y, z;
    };
    struct uint4
    {
        uint32 x, y, z, w;
    };

    using sint = sint32;
    struct sint2
    {
        sint32 x, y;
    };
    struct sint3
    {
        sint32 x, y, z;
    };
    struct sint4
    {
        sint32 x, y, z, w;
    };

    // <> Float2
    struct float2
    {
        float2() : x(0.f), y(0.f) {}
        float2(float x, float y) : x(x), y(y) {}

        float2& operator+=(const float2& right);
        float2& operator-=(const float2& right);
        float2& operator*=(const float factor);
        float2& operator/=(const float factor);

        float x, y;
    };

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
        float3() : x(0.f), y(0.f), z(0.f) {}
        float3(float x, float y, float z) : x(x), y(y), z(z) {}
        float3(float splat) : x(splat), y(splat), z(splat) {}

        float3& operator+=(const float3& right);
        float3& operator-=(const float3& right);
        float3& operator*=(const float right);
        float3& operator/=(const float right);

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
        float4() : x(0.f), y(0.f), z(0.f), w(1.f) {}
        float4(float x, float y, float z) : x(x), y(y), z(z), w(1.f) {}
        float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        float4(const float3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        float4(float splat) : x(splat), y(splat), z(splat), w(splat) {}

        float4& operator+=(const float4& right);
        float4& operator-=(const float4& right);
        float4& operator*=(const float right);
        float4& operator/=(const float right);

        float x, y, z, w;
    };

    float4 operator-(const float4& right);
    float4 operator+(const float4& left, const float4& right);
    float4 operator-(const float4& left, const float4& right);
    float4 operator*(const float4& left, const float factor);
    float4 operator/(const float4& left, const float right);

    // <> Float3x3
    struct float3x3
    {
        float3 rows[3];

        float3x3()
            : rows{
                  {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f},
              }
        {
        }

        float3x3(float r00,
                 float r01,
                 float r02,
                 float r10,
                 float r11,
                 float r12,
                 float r20,
                 float r21,
                 float r22)
            :

              rows{
                  {r00, r01, r02}, {r10, r11, r12}, {r20, r21, r22},
              }
        {
        }
    };

    float3 operator*(const float3x3& mat, const float3& vec);

    // <> Float4x4
    // Row-major 4x4 matrix
    // defaults to identity
    struct float4x4
    {
        union {
            struct
            {
                float _11, _12, _13, _14;
                float _21, _22, _23, _24;
                float _31, _32, _33, _34;
                float _41, _42, _43, _44;
            };
            float4 rows[4];
        };

        float4x4()
            : rows{{1.f, 0.f, 0.f, 0.f},
                   {0.f, 1.f, 0.f, 0.f},
                   {0.f, 0.f, 1.f, 0.f},
                   {0.f, 0.f, 0.f, 1.f}}
        {
        }

        float4x4(float r00,
                 float r01,
                 float r02,
                 float r03,
                 float r10,
                 float r11,
                 float r12,
                 float r13,
                 float r20,
                 float r21,
                 float r22,
                 float r23,
                 float r30,
                 float r31,
                 float r32,
                 float r33)
            :

              rows{{r00, r01, r02, r03},
                   {r10, r11, r12, r13},
                   {r20, r21, r22, r23},
                   {r30, r31, r32, r33}}
        {
        }
    };

    float4x4 operator*(const float4x4& left, const float4x4& right);
    float4 operator*(const float4x4& mat, const float4& vec);

#if CAMY_ENABLE_LOGGING > 0
    std::ostream& operator<<(std::ostream& stream, const float2& val);
    std::ostream& operator<<(std::ostream& stream, const float3& val);
    std::ostream& operator<<(std::ostream& stream, const float4& val);
#endif

    namespace API
    {
        CAMY_API float len(const float2& vec);
        CAMY_API float len(const float3& vec);
        CAMY_API float len(const float4& vec);

        CAMY_API float len2(const float2& vec);
        CAMY_API float len2(const float3& vec);
        CAMY_API float len2(const float4& vec);

        CAMY_API float dot(const float2& a, const float2& b);
        CAMY_API float dot(const float3& a, const float3& b);
        CAMY_API float dot(const float4& a, const float4& b);

        CAMY_API float3 cross(const float3& a, const float3& b);

        CAMY_API float2 normalize(const float2& vec);
        CAMY_API float3 normalize(const float3& vec);
        CAMY_API float4 normalize(const float4& vec);

        CAMY_API float3 mul(const float3x3& mat, const float3& vec);
        CAMY_API float4 mul(const float4x4& mat, const float4& vec);
        CAMY_API float4x4 mul(const float4x4& left, const float4x4& right);
        CAMY_API float4x4 transpose(const float4x4& mat);
        CAMY_API float4x4 invert(const float4x4& mat);

        CAMY_API float3x3 rotation(const float3& dir, float angle);
        CAMY_API float4x4 look_at(const float3& pos, const float3& at, const float3& up);
        CAMY_API float4x4 perspective(float fov, float ratio, float near_z, float far_z);
    }

    namespace colors
    {
        CAMY_INLINE float4 from_packed(uint32 packed)
        {
            return {static_cast<float>((packed >> 24) & 0xFF) / 255,
                    static_cast<float>((packed >> 16) & 0xFF) / 255,
                    static_cast<float>((packed >> 8) & 0xFF) / 255,
                    static_cast<float>((packed)&0xFF) / 255};
        }

        const float4 white = from_packed(0xFFFFFFFF);
        const float4 black = from_packed(0x000000FF);
        const float4 ivory = from_packed(0xFFFFF0FF);
        const float4 beige = from_packed(0xF5F5DCFF);
        const float4 wheat = from_packed(0xF5DeB3FF);
        const float4 tan = from_packed(0xD2B48CFF);
        const float4 khaki = from_packed(0xC3B091FF);
        const float4 silver = from_packed(0xC0C0C0FF);
        const float4 gray = from_packed(0x808080FF);
        const float4 charcoal = from_packed(0x464646FF);
        const float4 navy_blue = from_packed(0x000080FF);
        const float4 royal_blue = from_packed(0x084C9EFF);
        const float4 medium_blue = from_packed(0x0000CDFF);
        const float4 azure = from_packed(0x007FFFFF);
        const float4 cyan = from_packed(0x00FFFFFF);
        const float4 aquamarine = from_packed(0x7FFFD4FF);
        const float4 teal = from_packed(0x008080FF);
        const float4 forest_green = from_packed(0x228B22FF);
        const float4 olive = from_packed(0x808000FF);
        const float4 chartreuse = from_packed(0x7FFF00FF);
        const float4 lime = from_packed(0xBFFF00FF);
        const float4 golden = from_packed(0xFFD700FF);
        const float4 golden_rod = from_packed(0xDAA520FF);
        const float4 coral = from_packed(0xFF7F50FF);
        const float4 salmon = from_packed(0xFA8072FF);
        const float4 hot_pink = from_packed(0xFC9FC9FF);
        const float4 fuchsia = from_packed(0xFF77FFFF);
        const float4 puce = from_packed(0xCC8899FF);
        const float4 mauve = from_packed(0xE0B0FFFF);
        const float4 lavendere = from_packed(0xB57EDCFF);
        const float4 plum = from_packed(0x843179FF);
        const float4 indigo = from_packed(0x4B0082FF);
        const float4 maroon = from_packed(0x800000FF);
        const float4 crimson = from_packed(0xDC143CFF);
    }
}