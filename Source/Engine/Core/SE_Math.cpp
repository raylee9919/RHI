// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Math.h"

#include "Core/Core_Common.h"

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

    vec4::vec4(f32 f1, f32 f2, f32 f3, f32 f4)
    {
        lane = _mm_setr_ps(f1, f2, f3, f4);
    }

    void AABB::Expand(vec3 p)
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
        AABB aabb;
        aabb.min = vec3(F32_MAX);
        aabb.max = vec3(F32_MIN);
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

    m4x4 m4x4::Identity()
    {
        m4x4 m;
        m.rows[0].lane = _mm_setr_ps(1.f, 0.f, 0.f, 0.f);
        m.rows[1].lane = _mm_setr_ps(0.f, 1.f, 0.f, 0.f);
        m.rows[2].lane = _mm_setr_ps(0.f, 0.f, 1.f, 0.f);
        m.rows[3].lane = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
        return m;
    }

    Quaternion::Quaternion(f32 f1, f32 f2, f32 f3, f32 f4)
    {
        lane = _mm_setr_ps(f1, f2, f3, f4);
    }
}
