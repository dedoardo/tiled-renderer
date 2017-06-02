/* types.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/math.hpp>

// libc
#include <math.h>

#if defined(CAMY_OS_WINDOWS)
#include <DirectXMath.h>
#endif

#if CAMY_ENABLE_LOGGING > 0
#include <iomanip>
#endif

namespace camy
{
    float2& float2::operator+=(const float2& right)
    {
        x += right.x;
        y += right.y;
        return *this;
    }

    float2& float2::operator-=(const float2& right)
    {
        x -= right.x;
        y -= right.y;
        return *this;
    }

    float2& float2::operator*=(const float factor)
    {
        x *= factor;
        y *= factor;
        return *this;
    }

    float2& float2::operator/=(const float factor)
    {
        x /= factor;
        y /= factor;
        return *this;
    }

    bool operator==(const float2& left, const float2& right)
    {
        return left.x == right.x && left.y == right.y;
    }

    float2 operator-(const float2& right) { return float2(-right.x, -right.y); }

    float2 operator+(const float2& left, const float2& right)
    {
        return float2(left.x + right.x, left.y + right.y);
    }

    float2 operator-(const float2& left, const float2& right)
    {
        return float2(left.x - right.x, left.y - right.y);
    }

    float2 operator*(const float2& left, const float factor)
    {
        return float2(left.x * factor, left.y * factor);
    }

    float2 operator/(const float2& left, const float factor)
    {
        float inv = 1.f / factor;
        return float2(left.x * inv, left.y * inv);
    }

    // <> Float3
    float3& float3::operator+=(const float3& right)
    {
        x += right.x;
        y += right.y;
        z += right.z;
        return *this;
    }

    float3& float3::operator-=(const float3& right)
    {
        x -= right.x;
        y -= right.y;
        z -= right.z;
        return *this;
    }

    float3& float3::operator*=(const float factor)
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    float3& float3::operator/=(const float factor)
    {
        float inv = 1.f / factor;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }

    float3 operator-(const float3& right) { return float3(-right.x, -right.y, -right.z); }

    float3 operator+(const float3& left, const float3& right)
    {
        return float3(left.x + right.x, left.y + right.y, left.z + right.z);
    }

    float3 operator-(const float3& left, const float3& right)
    {
        return float3(left.x - right.x, left.y - right.y, left.z - right.z);
    }

    float3 operator*(const float3& left, const float factor)
    {
        return float3(left.x * factor, left.y * factor, left.z * factor);
    }

    float3 operator/(const float3& left, const float factor)
    {
        float inv = 1.f / factor;
        return float3(left.x * inv, left.y * inv, left.z * inv);
    }

    // <> Float4
    float4& float4::operator+=(const float4& right)
    {
        x += right.x;
        y += right.y;
        z += right.z;
        w += right.w;
        return *this;
    }

    float4& float4::operator-=(const float4& right)
    {
        x -= right.x;
        y -= right.y;
        z -= right.z;
        w -= right.w;
        return *this;
    }

    float4& float4::operator*=(const float factor)
    {
        x *= factor;
        y *= factor;
        z *= factor;
        w *= factor;
        return *this;
    }

    float4& float4::operator/=(const float factor)
    {
        float inv = 1.f / factor;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
        return *this;
    }

    float4 operator-(const float4& right) { return float4(-right.x, -right.y, -right.z); }

    float4 operator+(const float4& left, const float4& right)
    {
        return float4(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
    }

    float4 operator-(const float4& left, const float4& right)
    {
        return float4(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
    }

    float4 operator*(const float4& left, const float factor)
    {
        return float4(left.x * factor, left.y * factor, left.z * factor, left.w * factor);
    }

    float4 operator/(const float4& left, const float factor)
    {
        float inv = 1.f / factor;
        return left * inv;
    }

    float3 operator*(const float3x3& mat, const float3& vec)
    {
        return float3(API::dot(mat.rows[0], vec), API::dot(mat.rows[1], vec),
                      API::dot(mat.rows[2], vec));
    }

    float4x4 operator*(const float4x4& left, const float4x4& right)
    {
        return API::mul(left, right);
    }

    float4 operator*(const float4x4& mat, const float4& vec)
    {
        return float4(API::dot(mat.rows[0], vec), API::dot(mat.rows[1], vec),
                      API::dot(mat.rows[2], vec), API::dot(mat.rows[3], vec));
    }

#if CAMY_ENABLE_LOGGING > 0
    std::ostream& operator<<(std::ostream& stream, const float2& val)
    {
        stream << std::fixed << ::std::setprecision(5) << "<" << val.x << "," << val.y << ">";
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, const float3& val)
    {
        stream << std::fixed << ::std::setprecision(5) << "<" << val.x << "," << val.y << ","
               << val.z << ">";
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, const float4& val)
    {
        stream << std::fixed << ::std::setprecision(5) << "<" << val.x << "," << val.y << ","
               << val.z << "," << val.w << ">";
        return stream;
    }

#endif

    namespace API
    {
        float len(const float2& vec) { return sqrtf(vec.x * vec.x + vec.y * vec.y); }

        float len(const float3& vec)
        {
            return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
        }

        float len(const float4& vec) { return sqrtf(API::dot(vec, vec)); }

        float len2(const float2& vec) { return vec.x * vec.x + vec.y * vec.y; }

        float len2(const float3& vec) { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z; }

        float len2(const float4& vec) { return API::dot(vec, vec); }

        float dot(const float2& a, const float2& b) { return a.x * b.x + a.y * b.y; }

        float dot(const float3& a, const float3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

        float dot(const float4& a, const float4& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }

        float3 cross(const float3& a, const float3& b)
        {
            return float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
        }

        float2 normalize(const float2& vec)
        {
            float inv = 1.f / API::len(vec);
            return float2(vec.x * inv, vec.y * inv);
        }

        float3 normalize(const float3& vec)
        {
            float inv = 1.f / API::len(vec);
            return float3(vec.x * inv, vec.y * inv, vec.z * inv);
        }

        float4 normalize(const float4& vec)
        {
            float inv = 1.f / API::len(vec);
            return float4(vec.x * inv, vec.y * inv, vec.z * inv, vec.w * inv);
        }

        float3 mul(const float3x3& mat, const float3& vec)
        {
            return float3(API::dot(mat.rows[0], vec), API::dot(mat.rows[1], vec),
                          API::dot(mat.rows[2], vec));
        }

        float4 mul(const float4x4& mat, const float4& vec)
        {
            return float4(API::dot(mat.rows[0], vec), API::dot(mat.rows[1], vec),
                          API::dot(mat.rows[2], vec), API::dot(mat.rows[3], vec));
        }

        float4x4 mul(const float4x4& left, const float4x4& right)
        {
            float4x4 result;
            float* rf = (float*)&result;
            float* af = (float*)&left;
            float* bf = (float*)&right;
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    rf[i * 4 + j] = 0;
                    for (int k = 0; k < 4; ++k)
                        rf[i * 4 + j] += af[i * 4 + k] * bf[k * 4 + j];
                }
            }
            return result;
        }

        float4x4 transpose(const float4x4& mat)
        {
            return float4x4(mat.rows[0].x, mat.rows[1].x, mat.rows[2].x, mat.rows[3].x,
                            mat.rows[0].y, mat.rows[1].y, mat.rows[2].y, mat.rows[3].y,
                            mat.rows[0].z, mat.rows[1].z, mat.rows[2].z, mat.rows[3].z,
                            mat.rows[0].w, mat.rows[1].w, mat.rows[2].w, mat.rows[3].w);
        }

#if defined(CAMY_OS_WINDOWS)
        float4x4 invert(const float4x4& mat)
        {
            using namespace DirectX;
            XMMATRIX _mat = XMLoadFloat4x4((XMFLOAT4X4*)&mat);
            float4x4 ret;
            XMStoreFloat4x4((XMFLOAT4X4*)&ret, XMMatrixInverse(nullptr, _mat));
            return ret;
        }
#endif

        float3x3 rotation(const float3& dir, float angle)
        {
            float3 u = API::normalize(dir);
            float c = cosf(angle);
            float s = sinf(angle);
            float cc = 1.f - c;

            return float3x3(
                c + (u.x * u.x) * cc, u.y * u.x * cc + u.z * s, u.z * u.x * cc - u.y * s,
                u.x * u.y * cc - u.z * s, c + (u.y * u.y) * cc, u.z * u.y * cc + u.x * s,
                u.x * u.z * cc + u.y * s, u.y * u.z * cc - u.x * s, c + (u.z * u.z) * cc);
        }

        // http://stackoverflow.com/questions/349050/calculating-a-lookat-matrix
        // Translation + Rotation
        float4x4 look_at(const float3& pos, const float3& at, const float3& up)
        {
            // Building axis system
            float3 dir = at - pos;
            float3 r2 = API::normalize(dir);           // Z
            float3 r0 = API::normalize(cross(up, r2)); // X
            float3 r1 = API::cross(r2, r0);            // Y

            float d0 = API::dot(r0, -pos);
            float d1 = API::dot(r1, -pos);
            float d2 = API::dot(r2, -pos);

            return float4x4(r0.x, r1.x, r2.x, 0.f, r0.y, r1.y, r2.y, 0.f, r0.z, r1.z, r2.z, 0.f, d0,
                            d1, d2, 1.f);
        }

        float4x4 perspective(float fov, float ratio, float near_z, float far_z)
        {
            float sin_fov = sinf(fov * .5f);
            float cos_fov = cosf(fov * .5f);

            float h = cos_fov / sin_fov;
            float w = h / ratio;
            float range = far_z / (far_z - near_z);

            return float4x4(w, 0.f, 0.f, 0.f, 0.f, h, 0.f, 0.f, 0.f, 0.f, range, 1.f, 0.f, 0.f,
                            -range * near_z, 1.f);
        }
    }
}