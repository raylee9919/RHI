// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "SE_Basics.h"

#include <immintrin.h>

#define PI (3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844)

namespace Engine
{
    struct ENGINE_API vec2
    {
        f32 x, y;
    };

    struct ENGINE_API vec3
    {
        f32 x, y, z;

        vec3() = default;
        explicit vec3(f32 f1, f32 f2, f32 f3);
        explicit vec3(f32 f);
    };

    union ENGINE_API vec4
    {
        struct {
            union {
                vec3 xyz;
                struct { f32 x, y, z; };
            };
            f32 w;
        };
        __m128 lane;

        vec4() = default;
        explicit vec4(f32 f1, f32 f2, f32 f3, f32 f4);
        explicit vec4(vec3 f123, f32 f4);
    };

    union Quaternion
    {
        struct { f32 w, x, y, z; };
        __m128 lane;

        Quaternion() = default;
        explicit Quaternion(f32 f1, f32 f2, f32 f3, f32 f4);
    };

    struct Xform
    {
        vec3       translation;
        Quaternion rotation;
        vec3       scale;
    };

    union ENGINE_API m4x4
    {
        struct
        {
            f32 _11, _12, _13, _14;
            f32 _21, _22, _23, _24;
            f32 _31, _32, _33, _34;
            f32 _41, _42, _43, _44;
        };
        vec4 rows[4];
        f32 f[4][4];

        m4x4() = default;
        m4x4(Xform xform);

        static m4x4 Identity();
        static m4x4 Scale(f32 x, f32 y, f32 z);
        static m4x4 LookAtLH(vec3 from, vec3 at, vec3 up);
        static m4x4 PerspectiveLH(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
    };

    struct ENGINE_API AABB
    {
        vec3 min;
        vec3 max;

        void Expand(const vec3& p);

        static AABB RevInf();
    };

    FORCE_INLINE f32 DegreeToRadian(f32 f)
    {
        constexpr f32 d = (f32)PI / 180.f;
        return f * d;
    }

    ENGINE_API vec3 operator - (const vec3& l, const vec3& r);
    ENGINE_API f32 Dot(const vec3& l, const vec3& r);
    ENGINE_API vec3 Cross(const vec3& l, const vec3& r);
    ENGINE_API vec3 Normalize(const vec3& v);
    ENGINE_API m4x4 operator * (const m4x4& l, const m4x4& r);
    ENGINE_API vec4 operator * (const m4x4& l, const vec4& r);
    ENGINE_API vec4 operator *= (vec4& l, const f32& r);
}
