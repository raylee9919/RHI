// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Math.h"

#include "Core/SE_Basics.h"

namespace Engine
{
    vec3::vec3(f32 f)
    {
        x = f;
        y = f;
        z = f;
    }

    vec3::vec3(f32 f1, f32 f2, f32 f3)
    {
        x = f1;
        y = f2;
        z = f3;
    }

    vec3 operator + (const vec3& l, const vec3& r)
    {
        vec3 v;
        v.x = l.x + r.x;
        v.y = l.y + r.y;
        v.z = l.z + r.z;
        return v;
    }

    vec3 operator - (const vec3& l, const vec3& r)
    {
        vec3 v;
        v.x = l.x - r.x;
        v.y = l.y - r.y;
        v.z = l.z - r.z;
        return v;
    }

    vec3 operator * (const vec3& l, const f32 r)
    {
        vec3 v;
        v.x = l.x * r;
        v.y = l.y * r;
        v.z = l.z * r;
        return v;
    }

    vec3& operator += (vec3& l, const vec3 r)
    {
        l.x += r.x;
        l.y += r.y;
        l.z += r.z;
        return l;
    }

    vec3& operator -= (vec3& l, const vec3 r)
    {
        l.x -= r.x;
        l.y -= r.y;
        l.z -= r.z;
        return l;
    }

    f32 dot(const vec3& l, const vec3& r)
    {
        f32 f = l.x * r.x + l.y * r.y + l.z * r.z;
        return f;
    }

    vec3 cross(const vec3& l, const vec3& r)
    {
        vec3 v;
        v.x = l.y * r.z - l.z * r.y;
        v.y = l.z * r.x - l.x * r.z;
        v.z = l.x * r.y - l.y * r.x;
        return v;
    }

    vec3 normalize(const vec3& v)
    {
        vec3 result;

        f32 len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        CORE_ASSERT(len > 1e-8f);
        f32 rcp_len = 1.f / len;

        result.x = v.x * rcp_len;
        result.y = v.y * rcp_len;
        result.z = v.z * rcp_len;
        return result;
    }

    vec4::vec4(f32 f1, f32 f2, f32 f3, f32 f4)
    {
        lane = _mm_setr_ps(f1, f2, f3, f4);
    }

    vec4::vec4(vec3 f123, f32 f4)
    {
        lane = _mm_setr_ps(f123.x, f123.y, f123.z, f4);
    }

    void AABB::Expand(const vec3& p)
    {
        min.x = min(min.x, p.x);
        min.y = min(min.y, p.y);
        min.z = min(min.z, p.z);

        max.x = max(max.x, p.x);
        max.y = max(max.y, p.y);
        max.z = max(max.z, p.z);
    }

    AABB AABB::RevInf()
    {
        constexpr f32 float_max = 3.402823e+38f;
        AABB aabb;
        aabb.min = vec3( float_max);
        aabb.max = vec3(-float_max);
        return aabb;
    }

    m4x4::m4x4(Xform xform)
    {
        vec3 t = xform.translation;
        Quaternion r = xform.rotation;
        vec3 s = xform.scale;

        _11 = 1.0f - 2.0f * (r.y * r.y + r.z * r.z);
        _12 = 2.0f * (r.x * r.y - r.z * r.w);
        _13 = 2.0f * (r.x * r.z + r.y * r.w);
        _14 = t.x;
        rows[0].lane = _mm_mul_ps(rows[0].lane, _mm_setr_ps(s.x, s.x, s.x, 1));

        _21 = 2.0f * (r.x * r.y + r.z * r.w);
        _22 = 1.0f - 2.0f * (r.x * r.x + r.z * r.z);
        _23 = 2.0f * (r.y * r.z - r.x * r.w);
        _24 = t.y;
        rows[1].lane = _mm_mul_ps(rows[1].lane, _mm_setr_ps(s.y, s.y, s.y, 1));

        _31 = 2.0f * (r.x * r.z - r.y * r.w);
        _32 = 2.0f * (r.y * r.z + r.x * r.w);
        _33 = 1.0f - 2.0f * (r.x * r.x + r.y * r.y);
        _34 = t.z;
        rows[2].lane = _mm_mul_ps(rows[2].lane, _mm_setr_ps(s.z, s.z, s.z, 1));

        rows[3].lane = _mm_setr_ps(0, 0, 0, 1);
    }

    m4x4 operator * (const m4x4& L, const m4x4& R)
    {
        m4x4 m = {};

        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                for (int i = 0; i < 4; ++i) {
                    m.f[r][c] += L.f[r][i] * R.f[i][c];
                }
            }
        }

        return m;
    }

    vec4 operator * (const m4x4& l, const vec4& r)
    {
        vec4 v;
        v.x = l._11 * r.x + l._12 * r.y + l._13 * r.z + l._14 * r.w;
        v.y = l._21 * r.x + l._22 * r.y + l._23 * r.z + l._24 * r.w;
        v.z = l._31 * r.x + l._32 * r.y + l._33 * r.z + l._34 * r.w;
        v.w = l._41 * r.x + l._42 * r.y + l._43 * r.z + l._44 * r.w;
        return v;
    }

    vec4 operator * (const vec4& l, const f32 r)
    {
        vec4 v;
        __m128 s = _mm_set1_ps(r);
        v.lane = _mm_mul_ps(l.lane, s);
        return v;
    }

    vec4& operator *= (vec4& l, const f32 r)
    {
        l.lane = _mm_mul_ps(l.lane, _mm_set1_ps(r));
        return l;
    }

    vec4& operator += (vec4& l, vec4 r)
    {
        l.lane = _mm_add_ps(l.lane, r.lane);
        return l;
    }

    vec4& operator -= (vec4& l, vec4 r)
    {
        l.lane = _mm_sub_ps(l.lane, r.lane);
        return l;
    }

    m4x4 m4x4::identity()
    {
        m4x4 m;
        m.rows[0].lane = _mm_setr_ps(1.f, 0.f, 0.f, 0.f);
        m.rows[1].lane = _mm_setr_ps(0.f, 1.f, 0.f, 0.f);
        m.rows[2].lane = _mm_setr_ps(0.f, 0.f, 1.f, 0.f);
        m.rows[3].lane = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
        return m;
    }

    m4x4 m4x4::scale(f32 x, f32 y, f32 z)
    {
        m4x4 m;
        m.rows[0].lane = _mm_setr_ps(  x, 0.f, 0.f, 0.f);
        m.rows[1].lane = _mm_setr_ps(0.f,   y, 0.f, 0.f);
        m.rows[2].lane = _mm_setr_ps(0.f, 0.f,   z, 0.f);
        m.rows[3].lane = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
        return m;
    }

    m4x4 m4x4::look_at_lh(vec3 eye, vec3 at, vec3 up)
    {
        m4x4 m;

        vec3 z = normalize(at - eye);
        vec3 x = normalize(cross(z, up));
        vec3 y = cross(x, z);

        m.rows[0] = vec4(x, -dot(x, eye));
        m.rows[1] = vec4(y, -dot(y, eye));
        m.rows[2] = vec4(z, -dot(z, eye));
        m.rows[3] = vec4(0.f, 0.f, 0.f, 1.f);

        return m;
    }

    m4x4 m4x4::look_to_lh(vec3 eye, vec3 dir, vec3 up)
    {
        return look_at_lh(eye, eye + dir, up);
    }

    m4x4 m4x4::perspective_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z)
    {
        m4x4 m;
        CORE_ASSERT(near_z > 0.f && far_z > 0.f && (near_z - far_z != 0.f));

        f32 n = near_z;
        f32 f = far_z;

        f32 rcp_tan = 1.f / tanf(fov * 0.5f);
        m.rows[0] = vec4(rcp_tan, 0.f, 0.f, 0.f);
        m.rows[1] = vec4(0.f, rcp_tan / aspect_ratio, 0.f, 0.f);
        m.rows[2] = vec4(0.f, 0.f, f / (f - n), n*f / (n - f));
        m.rows[3] = vec4(0.f, 0.f, 1.f, 0.f);

        return m;
    }

    Quaternion::Quaternion(f32 f1, f32 f2, f32 f3, f32 f4)
    {
        lane = _mm_setr_ps(f1, f2, f3, f4);
    }

    ENGINE_API f32 fmod(const f32 l, const f32 r)
    {
        f32 f = fmodf(l, r);
        return f;
    }

    ENGINE_API m4x4 x_rotation(const f32 radian)
    {
        f32 c = cos(radian);
        f32 s = sin(radian);

        m4x4 m;

        m._11 = 1.0f;
        m._12 = 0.0f;
        m._13 = 0.0f;
        m._14 = 0.0f;

        m._21 = 0.0f;
        m._22 = c;
        m._23 = s;
        m._24 = 0.0f;

        m._31 = 0.0f;
        m._32 = -s;
        m._33 = c;
        m._34 = 0.0f;

        m._41 = 0.0f;
        m._42 = 0.0f;
        m._43 = 0.0f;
        m._44 = 1.0f;

        return m;
    }

    ENGINE_API m4x4 y_rotation(const f32 radian)
    {
        f32 c = cos(radian);
        f32 s = sin(radian);

        m4x4 m;

        m._11 = c;
        m._12 = 0.0f;
        m._13 = -s;
        m._14 = 0.0f;

        m._21 = 0.0f;
        m._22 = 1.0f;
        m._23 = 0.0f;
        m._24 = 0.0f;

        m._31 = s;
        m._32 = 0.0f;
        m._33 = c;
        m._34 = 0.0f;

        m._41 = 0.0f;
        m._42 = 0.0f;
        m._43 = 0.0f;
        m._44 = 1.0f;

        return m;
    }

    ENGINE_API m4x4 z_rotation(const f32 radian)
    {
        f32 c = cos(radian);
        f32 s = sin(radian);

        m4x4 m;

        m._11 = c;
        m._12 = s;
        m._13 = 0.0f;
        m._14 = 0.0f;

        m._21 = -s;
        m._22 = c;
        m._23 = 0.0f;
        m._24 = 0.0f;

        m._31 = 0.0f;
        m._32 = 0.0f;
        m._33 = 1.0f;
        m._34 = 0.0f;

        m._41 = 0.0f;
        m._42 = 0.0f;
        m._43 = 0.0f;
        m._44 = 1.0f;

        return m;
    }
}
