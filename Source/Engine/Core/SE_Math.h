// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "SE_Basics.h"

#include <immintrin.h>

#define PI (3.14159265358979323846264338327950288f)

namespace Engine
{
    struct ENGINE_API vec2
    {
        f32 x, y;
    };

    struct ENGINE_API vec2u {
        u32 x, y;
    };

    union ENGINE_API vec3
    {
        struct { f32 x, y, z; };
        f32 f[3];

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

    union ENGINE_API m3x3 {
        struct {
            f32 _11, _12, _13;
            f32 _21, _22, _23;
            f32 _31, _32, _33;
        };

        vec3 rows[3];
        f32 coef[3][3];
        f32 f[9];
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
        f32 coef[4][4];
        f32 f[4][4];

        m4x4() = default;

        static m4x4 identity();
        static m4x4 scale(f32 x, f32 y, f32 z);
    };

    struct ENGINE_API AABB
    {
        vec3 min;
        vec3 max;

        void Expand(const vec3& p);

        static AABB RevInf();
    };

    FORCE_INLINE f32 to_radian(f32 f) {
        constexpr f32 d = (f32)PI / 180.f;
        return f * d;
    }

    FORCE_INLINE f32 to_degree(f32 f) {
        constexpr f32 d = 180.0f / (f32)PI;
        return f * d;
    }

    ENGINE_API vec3 operator + (const vec3& l, const vec3& r);
    ENGINE_API vec3 operator - (const vec3& l, const vec3& r);
    ENGINE_API vec3 operator - (vec3 v);
    ENGINE_API vec3 operator * (const vec3& l, const f32 r);
    ENGINE_API vec3& operator += (vec3& l, const vec3 r);
    ENGINE_API vec3& operator -= (vec3& l, const vec3 r);
    ENGINE_API f32 dot(const vec3& l, const vec3& r);
    ENGINE_API vec3 cross(const vec3& l, const vec3& r);
    ENGINE_API vec3 normalize(const vec3& v);

    ENGINE_API m3x3 operator * (f32 L, const m3x3& R);
    ENGINE_API m3x3 operator + (const m3x3& L, const m3x3& R);
    ENGINE_API m3x3 operator - (const m3x3& L, const m3x3& R);
    ENGINE_API m3x3 operator -= (m3x3& L, const m3x3& R);
    ENGINE_API m3x3 operator * (const m3x3& L, const m3x3& R);
    ENGINE_API m4x4 operator * (const m4x4& l, const m4x4& r);
    ENGINE_API vec4 operator * (const m4x4& l, const vec4& r);
    ENGINE_API vec4 operator * (const vec4& l, const f32 r);

    ENGINE_API vec4& operator *= (vec4& l, const f32 r);
    ENGINE_API vec4& operator += (vec4& l, const vec4 r);
    ENGINE_API vec4& operator -= (vec4& l, const vec4 r);

    ENGINE_API f32 fmod(const f32 l, const f32 r);

    ENGINE_API m4x4 look_at_lh(vec3 eye, vec3 at, vec3 up);
    ENGINE_API m4x4 look_to_lh(vec3 eye, vec3 dir, vec3 up);
    ENGINE_API m4x4 look_at_rh(vec3 eye, vec3 at, vec3 up);
    ENGINE_API m4x4 look_to_rh(vec3 eye, vec3 dir, vec3 up);
    ENGINE_API m4x4 perspective_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
    ENGINE_API m4x4 perspective_rh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);

    ENGINE_API m4x4 x_rotation(const f32 radian);
    ENGINE_API m4x4 y_rotation(const f32 radian);
    ENGINE_API m4x4 z_rotation(const f32 radian);

    ENGINE_API m4x4 translation_matrix(const vec3& v);
    ENGINE_API m4x4 rotation_matrix(const Quaternion& q);
    ENGINE_API m4x4 scale_matrix(const vec3& v);

    ENGINE_API m3x3 to_m3x3(m4x4 M);
    ENGINE_API m4x4 to_m4x4(vec3 T, Quaternion R, vec3 S);
    ENGINE_API m4x4 to_m4x4(Xform xform);
    ENGINE_API m3x3 transpose(const m3x3& m);
    ENGINE_API m4x4 transpose(const m4x4& m);

    ENGINE_API Quaternion quaternion_from_euler(vec3 euler);
    ENGINE_API vec3 euler_from_quaternion(Quaternion q);
    ENGINE_API vec3 to_degree(const vec3& radian);
    ENGINE_API vec3 to_radian(const vec3& degree);
}
