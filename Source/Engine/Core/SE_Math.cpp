// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Math.h"

#include "Core/SE_Basics.h"

namespace Engine
{
    vec3::vec3(f32 f) {
        x = f;
        y = f;
        z = f;
    }

    vec3::vec3(f32 f1, f32 f2, f32 f3) {
        x = f1;
        y = f2;
        z = f3;
    }

    vec3 operator + (const vec3& l, const vec3& r) {
        vec3 v;
        v.x = l.x + r.x;
        v.y = l.y + r.y;
        v.z = l.z + r.z;
        return v;
    }

    vec3 operator - (const vec3& l, const vec3& r) {
        vec3 v;
        v.x = l.x - r.x;
        v.y = l.y - r.y;
        v.z = l.z - r.z;
        return v;
    }

    vec3 operator - (vec3 v) {
        v.x = -v.x;
        v.y = -v.y;
        v.z = -v.z;
        return v;
    }

    vec3 operator * (const vec3& l, const f32 r) {
        vec3 v;
        v.x = l.x * r;
        v.y = l.y * r;
        v.z = l.z * r;
        return v;
    }

    vec3& operator += (vec3& l, const vec3 r) {
        l.x += r.x;
        l.y += r.y;
        l.z += r.z;
        return l;
    }

    vec3& operator -= (vec3& l, const vec3 r) {
        l.x -= r.x;
        l.y -= r.y;
        l.z -= r.z;
        return l;
    }

    f32 dot(const vec3& l, const vec3& r) {
        f32 f = l.x * r.x + l.y * r.y + l.z * r.z;
        return f;
    }

    vec3 cross(const vec3& l, const vec3& r) {
        vec3 v;
        v.x = l.y * r.z - l.z * r.y;
        v.y = l.z * r.x - l.x * r.z;
        v.z = l.x * r.y - l.y * r.x;
        return v;
    }

    vec3 normalize(const vec3& v) {
        vec3 result;

        f32 len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        CORE_ASSERT(len > 1e-8f);
        f32 rcp_len = 1.f / len;

        result.x = v.x * rcp_len;
        result.y = v.y * rcp_len;
        result.z = v.z * rcp_len;
        return result;
    }

    vec4::vec4(f32 f1, f32 f2, f32 f3, f32 f4) {
        lane = _mm_setr_ps(f1, f2, f3, f4);
    }

    vec4::vec4(vec3 f123, f32 f4) {
        lane = _mm_setr_ps(f123.x, f123.y, f123.z, f4);
    }

    void AABB::Expand(const vec3& p) {
        min.x = min(min.x, p.x);
        min.y = min(min.y, p.y);
        min.z = min(min.z, p.z);

        max.x = max(max.x, p.x);
        max.y = max(max.y, p.y);
        max.z = max(max.z, p.z);
    }

    AABB AABB::RevInf() {
        constexpr f32 float_max = 3.402823e+38f;
        AABB aabb;
        aabb.min = vec3( float_max);
        aabb.max = vec3(-float_max);
        return aabb;
    }

    m3x3 operator * (f32 L, const m3x3& R) {
        m3x3 m = R;

        m._11 *= L;
        m._21 *= L;
        m._31 *= L;

        m._12 *= L;
        m._22 *= L;
        m._32 *= L;

        m._13 *= L;
        m._23 *= L;
        m._33 *= L;

        return m;
    }

    m3x3 operator + (const m3x3& L, const m3x3& R) {
        m3x3 m;

        m._11 = L._11 + R._11;
        m._21 = L._21 + R._21;
        m._31 = L._31 + R._31;

        m._12 = L._12 + R._12;
        m._22 = L._22 + R._22;
        m._32 = L._32 + R._32;

        m._13 = L._13 + R._13;
        m._23 = L._23 + R._23;
        m._33 = L._33 + R._33;

        return m;
    }

    m3x3 operator - (const m3x3& L, const m3x3& R) {
        m3x3 m;

        m._11 = L._11 - R._11;
        m._21 = L._21 - R._21;
        m._31 = L._31 - R._31;

        m._12 = L._12 - R._12;
        m._22 = L._22 - R._22;
        m._32 = L._32 - R._32;

        m._13 = L._13 - R._13;
        m._23 = L._23 - R._23;
        m._33 = L._33 - R._33;

        return m;
    }

    m3x3& operator -= (m3x3& L, const m3x3& R) {
        L._11 = L._11 - R._11;
        L._21 = L._21 - R._21;
        L._31 = L._31 - R._31;

        L._12 = L._12 - R._12;
        L._22 = L._22 - R._22;
        L._32 = L._32 - R._32;

        L._13 = L._13 - R._13;
        L._23 = L._23 - R._23;
        L._33 = L._33 - R._33;

        return L;
    }

    m3x3 operator * (const m3x3& L, const m3x3& R) {
        m3x3 m = {};

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                for (int i = 0; i < 3; ++i) {
                    m.coef[r][c] += L.coef[r][i] * R.coef[i][c];
                }
            }
        }

        return m;
    }

    m4x4 operator * (const m4x4& L, const m4x4& R) {
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

    vec4 operator * (const m4x4& l, const vec4& r) {
        vec4 v;
        v.x = l._11 * r.x + l._12 * r.y + l._13 * r.z + l._14 * r.w;
        v.y = l._21 * r.x + l._22 * r.y + l._23 * r.z + l._24 * r.w;
        v.z = l._31 * r.x + l._32 * r.y + l._33 * r.z + l._34 * r.w;
        v.w = l._41 * r.x + l._42 * r.y + l._43 * r.z + l._44 * r.w;
        return v;
    }

    vec4 operator * (const vec4& l, const f32 r) {
        vec4 v;
        __m128 s = _mm_set1_ps(r);
        v.lane = _mm_mul_ps(l.lane, s);
        return v;
    }

    vec4& operator *= (vec4& l, const f32 r) {
        l.lane = _mm_mul_ps(l.lane, _mm_set1_ps(r));
        return l;
    }

    vec4& operator += (vec4& l, vec4 r) {
        l.lane = _mm_add_ps(l.lane, r.lane);
        return l;
    }

    vec4& operator -= (vec4& l, vec4 r) {
        l.lane = _mm_sub_ps(l.lane, r.lane);
        return l;
    }

    m4x4 m4x4::identity() {
        m4x4 m;
        m.rows[0].lane = _mm_setr_ps(1.f, 0.f, 0.f, 0.f);
        m.rows[1].lane = _mm_setr_ps(0.f, 1.f, 0.f, 0.f);
        m.rows[2].lane = _mm_setr_ps(0.f, 0.f, 1.f, 0.f);
        m.rows[3].lane = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
        return m;
    }

    m4x4 m4x4::scale(f32 x, f32 y, f32 z) {
        m4x4 m;
        m.rows[0].lane = _mm_setr_ps(  x, 0.f, 0.f, 0.f);
        m.rows[1].lane = _mm_setr_ps(0.f,   y, 0.f, 0.f);
        m.rows[2].lane = _mm_setr_ps(0.f, 0.f,   z, 0.f);
        m.rows[3].lane = _mm_setr_ps(0.f, 0.f, 0.f, 1.f);
        return m;
    }

    m4x4 look_at_lh(vec3 eye, vec3 at, vec3 up) {
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

    m4x4 look_to_lh(vec3 eye, vec3 dir, vec3 up) {
        return look_at_lh(eye, eye + dir, up);
    }

    m4x4 look_at_rh(vec3 eye, vec3 at, vec3 up) {
        m4x4 m;

        vec3 z = normalize(eye - at);
        vec3 x = normalize(cross(up, z));
        vec3 y = cross(z, x);

        m.rows[0] = vec4(x, -dot(x, eye));
        m.rows[1] = vec4(y, -dot(y, eye));
        m.rows[2] = vec4(z, -dot(z, eye));
        m.rows[3] = vec4(0.f, 0.f, 0.f, 1.f);

        return m;
    }

    m4x4 look_to_rh(vec3 eye, vec3 dir, vec3 up) {
        return look_at_rh(eye, eye + dir, up);
    }

    m4x4 perspective_lh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) {
        m4x4 m;
        CORE_ASSERT(near_z > 0.f && far_z > 0.f && (near_z - far_z != 0.f));

        f32 n = near_z;
        f32 f = far_z;

        f32 rcp_tan = 1.f / tanf(fov * 0.5f);
        m.rows[0] = vec4(rcp_tan, 0.f, 0.f, 0.f);
        m.rows[1] = vec4(0.f, rcp_tan / aspect_ratio, 0.f, 0.f);
        m.rows[2] = vec4(0.f, 0.f, f / (f - n), (n * f) / (n - f));
        m.rows[3] = vec4(0.f, 0.f, 1.f, 0.f);

        return m;
    }

    // @Todo: Correctness
    m4x4 perspective_rh(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) {
        m4x4 m;
        CORE_ASSERT(near_z > 0.f && far_z > 0.f && (near_z - far_z != 0.f));

        f32 n = near_z;
        f32 f = far_z;

        f32 rcp_tan = 1.f / tanf(fov * 0.5f);

        m.rows[0] = vec4(rcp_tan, 0.f, 0.f, 0.f);
        m.rows[1] = vec4(0.f, rcp_tan / aspect_ratio, 0.f, 0.f);
        m.rows[2] = vec4(0.f, 0.f, f / (n - f), (n * f) / (n - f));
        m.rows[3] = vec4(0.f, 0.f, -1.f, 0.f);

        return m;
    }

    Quaternion::Quaternion(f32 f1, f32 f2, f32 f3, f32 f4) {
        lane = _mm_setr_ps(f1, f2, f3, f4);
    }

    f32 fmod(const f32 l, const f32 r) {
        f32 f = fmodf(l, r);
        return f;
    }

    m4x4 x_rotation(const f32 radian) {
        f32 c = cos(radian);
        f32 s = sin(radian);

        m4x4 m;

        m._11 = 1.0f;
        m._12 = 0.0f;
        m._13 = 0.0f;
        m._14 = 0.0f;

        m._21 = 0.0f;
        m._22 = c;
        m._23 =-s;
        m._24 = 0.0f;

        m._31 = 0.0f;
        m._32 = s;
        m._33 = c;
        m._34 = 0.0f;

        m._41 = 0.0f;
        m._42 = 0.0f;
        m._43 = 0.0f;
        m._44 = 1.0f;

        return m;
    }

    m4x4 y_rotation(const f32 radian) {
        f32 c = cos(radian);
        f32 s = sin(radian);

        m4x4 m;

        m._11 = c;
        m._12 = 0.0f;
        m._13 = s;
        m._14 = 0.0f;

        m._21 = 0.0f;
        m._22 = 1.0f;
        m._23 = 0.0f;
        m._24 = 0.0f;

        m._31 =-s;
        m._32 = 0.0f;
        m._33 = c;
        m._34 = 0.0f;

        m._41 = 0.0f;
        m._42 = 0.0f;
        m._43 = 0.0f;
        m._44 = 1.0f;

        return m;
    }

    m4x4 z_rotation(const f32 radian) {
        f32 c = cos(radian);
        f32 s = sin(radian);

        m4x4 m;

        m._11 = c;
        m._12 =-s;
        m._13 = 0.0f;
        m._14 = 0.0f;

        m._21 = s;
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

    m4x4 translation_matrix(const vec3& v) {
        m4x4 m = m4x4::identity();

        m._14 = v.x;
        m._24 = v.y;
        m._34 = v.z;

        return m;
    }

    m4x4 rotation_matrix(const Quaternion& q) {
        m4x4 m = {};

        m._44 = 1.0f;

        f32 xs = q.x * 2.0f;
        f32 ys = q.y * 2.0f;
        f32 zs = q.z * 2.0f;

        f32 wx = q.w * xs;
        f32 wy = q.w * ys;
        f32 wz = q.w * zs;

        f32 _xx = q.x * xs;
        f32 xy = q.x * ys;
        f32 xz = q.x * zs;

        f32 yy = q.y * ys;
        f32 yz = q.y * zs;
        f32 zz = q.z * zs;

        m._11 = 1.0f - (yy + zz);
        m._12 = xy - wz;
        m._13 = xz + wy;

        m._21 = xy + wz;
        m._22 = 1.0f - (_xx + zz);
        m._23 = yz - wx;

        m._31 = xz - wy;
        m._32 = yz + wx;
        m._33 = 1.0f - (_xx + yy);

        return m;
    }

    m4x4 scale_matrix(const vec3& v) {
        m4x4 m = m4x4::identity();

        m._11 = v.x;
        m._22 = v.y;
        m._33 = v.z;

        return m;
    }

    m3x3 to_m3x3(m4x4 M) {
        m3x3 result;

        result._11 = M._11;
        result._12 = M._12;
        result._13 = M._13;

        result._21 = M._21;
        result._22 = M._22;
        result._23 = M._23;

        result._31 = M._31;
        result._32 = M._32;
        result._33 = M._33;

        return result;
    }

    m4x4 to_m4x4(vec3 T, Quaternion R, vec3 S) {
        m4x4 result = translation_matrix(T) * rotation_matrix(R) * scale_matrix(S);
        return result;
    }

    m4x4 to_m4x4(Xform xform) {
        return to_m4x4(xform.translation, xform.rotation, xform.scale);
    }

    m3x3 transpose(const m3x3& m) {
        m3x3 result;

        result._11 = m._11;
        result._12 = m._21;
        result._13 = m._31;

        result._21 = m._12;
        result._22 = m._22;
        result._23 = m._32;

        result._31 = m._13;
        result._32 = m._23;
        result._33 = m._33;

        return result;
    }

    m4x4 transpose(const m4x4& m) {
        m4x4 result;

        result._11 = m._11;
        result._12 = m._21;
        result._13 = m._31;
        result._14 = m._41;

        result._21 = m._12;
        result._22 = m._22;
        result._23 = m._32;
        result._24 = m._42;

        result._31 = m._13;
        result._32 = m._23;
        result._33 = m._33;
        result._34 = m._43;

        result._41 = m._14;
        result._42 = m._24;
        result._43 = m._34;
        result._44 = m._44;

        return result;
    }

    // @Correctness
    Quaternion quaternion_from_euler(vec3 euler) {
        float rx = euler.x;
        float ry = euler.y;
        float rz = euler.z;

        float cr = cosf(rx * 0.5f);
        float sr = sinf(rx * 0.5f);
        float cp = cosf(ry * 0.5f);
        float sp = sinf(ry * 0.5f);
        float cy = cosf(rz * 0.5f);
        float sy = sinf(rz * 0.5f);

        Quaternion q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;

        return q;
    }

    // @Correctness
    vec3 euler_from_quaternion(Quaternion q) {
        vec3 euler;

        // Roll (X-axis rotation)
        f32 sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
        f32 cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        euler.x = atan2f(sinr_cosp, cosr_cosp);

        // Pitch (Y-axis rotation)
        f32 sinp = 2.0f * (q.w * q.y - q.z * q.x);

        if (fabsf(sinp) >= 1.0f)
            euler.y = copysignf(PI * 0.5f, sinp); // use 90 degrees if out of range
        else
            euler.y = asinf(sinp);

        // Yaw (Z-axis rotation)
        f32 siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        f32 cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        euler.z = atan2f(siny_cosp, cosy_cosp);

        return euler;
    }

    vec3 to_degree(const vec3& radian){ 
        vec3 v;
        v.x = radian.x * (180.0f / PI);
        v.y = radian.y * (180.0f / PI);
        v.z = radian.z * (180.0f / PI);
        return v;
    }

    vec3 to_radian(const vec3& degree) {
        vec3 v;
        v.x = degree.x * (PI / 180.0f);
        v.y = degree.y * (PI / 180.0f);
        v.z = degree.z * (PI / 180.0f);
        return v;
    }

    m4x4 inverse(const m4x4& m) {
        f32 A2323 = m.coef[2][2] * m.coef[3][3] - m.coef[2][3] * m.coef[3][2];
        f32 A1323 = m.coef[2][1] * m.coef[3][3] - m.coef[2][3] * m.coef[3][1];
        f32 A1223 = m.coef[2][1] * m.coef[3][2] - m.coef[2][2] * m.coef[3][1];
        f32 A0323 = m.coef[2][0] * m.coef[3][3] - m.coef[2][3] * m.coef[3][0];
        f32 A0223 = m.coef[2][0] * m.coef[3][2] - m.coef[2][2] * m.coef[3][0];
        f32 A0123 = m.coef[2][0] * m.coef[3][1] - m.coef[2][1] * m.coef[3][0];
        f32 A2313 = m.coef[1][2] * m.coef[3][3] - m.coef[1][3] * m.coef[3][2];
        f32 A1313 = m.coef[1][1] * m.coef[3][3] - m.coef[1][3] * m.coef[3][1];
        f32 A1213 = m.coef[1][1] * m.coef[3][2] - m.coef[1][2] * m.coef[3][1];
        f32 A2312 = m.coef[1][2] * m.coef[2][3] - m.coef[1][3] * m.coef[2][2];
        f32 A1312 = m.coef[1][1] * m.coef[2][3] - m.coef[1][3] * m.coef[2][1];
        f32 A1212 = m.coef[1][1] * m.coef[2][2] - m.coef[1][2] * m.coef[2][1];
        f32 A0313 = m.coef[1][0] * m.coef[3][3] - m.coef[1][3] * m.coef[3][0];
        f32 A0213 = m.coef[1][0] * m.coef[3][2] - m.coef[1][2] * m.coef[3][0];
        f32 A0312 = m.coef[1][0] * m.coef[2][3] - m.coef[1][3] * m.coef[2][0];
        f32 A0212 = m.coef[1][0] * m.coef[2][2] - m.coef[1][2] * m.coef[2][0];
        f32 A0113 = m.coef[1][0] * m.coef[3][1] - m.coef[1][1] * m.coef[3][0];
        f32 A0112 = m.coef[1][0] * m.coef[2][1] - m.coef[1][1] * m.coef[2][0];

        f32 det = (m.coef[0][0] * ( m.coef[1][1] * A2323 - m.coef[1][2] * A1323 + m.coef[1][3] * A1223 ) - 
                   m.coef[0][1] * ( m.coef[1][0] * A2323 - m.coef[1][2] * A0323 + m.coef[1][3] * A0223 ) +
                   m.coef[0][2] * ( m.coef[1][0] * A1323 - m.coef[1][1] * A0323 + m.coef[1][3] * A0123 ) -
                   m.coef[0][3] * ( m.coef[1][0] * A1223 - m.coef[1][1] * A0223 + m.coef[1][2] * A0123 ));
        det = 1.f / det;

        m4x4 result;
        result._11 = det *   ( m.coef[1][1] * A2323 - m.coef[1][2] * A1323 + m.coef[1][3] * A1223 );
        result._12 = det * - ( m.coef[0][1] * A2323 - m.coef[0][2] * A1323 + m.coef[0][3] * A1223 );
        result._13 = det *   ( m.coef[0][1] * A2313 - m.coef[0][2] * A1313 + m.coef[0][3] * A1213 );
        result._14 = det * - ( m.coef[0][1] * A2312 - m.coef[0][2] * A1312 + m.coef[0][3] * A1212 );
        result._21 = det * - ( m.coef[1][0] * A2323 - m.coef[1][2] * A0323 + m.coef[1][3] * A0223 );
        result._22 = det *   ( m.coef[0][0] * A2323 - m.coef[0][2] * A0323 + m.coef[0][3] * A0223 );
        result._23 = det * - ( m.coef[0][0] * A2313 - m.coef[0][2] * A0313 + m.coef[0][3] * A0213 );
        result._24 = det *   ( m.coef[0][0] * A2312 - m.coef[0][2] * A0312 + m.coef[0][3] * A0212 );
        result._31 = det *   ( m.coef[1][0] * A1323 - m.coef[1][1] * A0323 + m.coef[1][3] * A0123 );
        result._32 = det * - ( m.coef[0][0] * A1323 - m.coef[0][1] * A0323 + m.coef[0][3] * A0123 );
        result._33 = det *   ( m.coef[0][0] * A1313 - m.coef[0][1] * A0313 + m.coef[0][3] * A0113 );
        result._34 = det * - ( m.coef[0][0] * A1312 - m.coef[0][1] * A0312 + m.coef[0][3] * A0112 );
        result._41 = det * - ( m.coef[1][0] * A1223 - m.coef[1][1] * A0223 + m.coef[1][2] * A0123 );
        result._42 = det *   ( m.coef[0][0] * A1223 - m.coef[0][1] * A0223 + m.coef[0][2] * A0123 );
        result._43 = det * - ( m.coef[0][0] * A1213 - m.coef[0][1] * A0213 + m.coef[0][2] * A0113 );
        result._44 = det *   ( m.coef[0][0] * A1212 - m.coef[0][1] * A0212 + m.coef[0][2] * A0112 );

        return result;
    }
}
