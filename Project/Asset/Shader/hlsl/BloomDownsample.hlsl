// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Push_Constants {
    uint input_id;
    uint output_id;
    uint bilinear_clamp_id;
};
PUSH_CONSTANTS(Push_Constants, push);

float luminance(float3 c)
{
    c = max(c, 0.0); // prevent negative HDR weirdness
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

float karis_weight(float3 c)
{
    float l = luminance(c);

    // guard against NaN/Inf
    if (!isfinite(l))
        l = 0.0;

    return rcp(1.0 + l);
}

float3 sanitize(float3 c)
{
    return isfinite(c.x) && isfinite(c.y) && isfinite(c.z)
        ? c
        : 0.0;
}

float3 karis_average(float3 a, float3 b, float3 c, float3 d)
{
    a = sanitize(a);
    b = sanitize(b);
    c = sanitize(c);
    d = sanitize(d);

    float wa = karis_weight(a);
    float wb = karis_weight(b);
    float wc = karis_weight(c);
    float wd = karis_weight(d);

    float sum = wa + wb + wc + wd;

    // avoid divide-by-zero / NaN
    sum = max(sum, 1e-5);

    return (a * wa + b * wb + c * wc + d * wd) / sum;
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D   <float3> input          = ResourceDescriptorHeap[push.input_id];
    RWTexture2D <float3> output         = ResourceDescriptorHeap[push.output_id];
    SamplerState         bilinear_clamp = SamplerDescriptorHeap[push.bilinear_clamp_id];

    uint2 out_dims;
    output.GetDimensions(out_dims.x, out_dims.y);
    if (any(dtid.xy >= out_dims))
        return;

    uint2 in_dims;
    input.GetDimensions(in_dims.x, in_dims.y);
    float2 texel = 1.0 / float2(in_dims);
    float2 uv    = (float2(dtid.xy) + 0.5) * texel * 2.0;

    // 13-tap filter (Jimenez, SIGGRAPH 2014)
    //
    //  [a] . [b] . [c]
    //  . [e] . [f] .
    //  [h] . [i] . [j]
    //  . [k] . [l] .
    //  [m] . [n] . [o]

    float3 a = input.SampleLevel(bilinear_clamp, uv + float2(-2, -2) * texel, 0);
    float3 b = input.SampleLevel(bilinear_clamp, uv + float2( 0, -2) * texel, 0);
    float3 c = input.SampleLevel(bilinear_clamp, uv + float2( 2, -2) * texel, 0);

    float3 e = input.SampleLevel(bilinear_clamp, uv + float2(-1, -1) * texel, 0);
    float3 f = input.SampleLevel(bilinear_clamp, uv + float2( 1, -1) * texel, 0);

    float3 h = input.SampleLevel(bilinear_clamp, uv + float2(-2,  0) * texel, 0);
    float3 i = input.SampleLevel(bilinear_clamp, uv + float2( 0,  0) * texel, 0);
    float3 j = input.SampleLevel(bilinear_clamp, uv + float2( 2,  0) * texel, 0);

    float3 k = input.SampleLevel(bilinear_clamp, uv + float2(-1,  1) * texel, 0);
    float3 l = input.SampleLevel(bilinear_clamp, uv + float2( 1,  1) * texel, 0);

    float3 m = input.SampleLevel(bilinear_clamp, uv + float2(-2,  2) * texel, 0);
    float3 n = input.SampleLevel(bilinear_clamp, uv + float2( 0,  2) * texel, 0);
    float3 o = input.SampleLevel(bilinear_clamp, uv + float2( 2,  2) * texel, 0);

    // Karis average on each 2x2 group to suppress fireflies
    float3 g0 = karis_average(e, f, k, l);      // center       weight 0.5
    float3 g1 = karis_average(a, b, e, f);      // top-left     weight 0.125
    float3 g2 = karis_average(b, c, f, j);      // top-right    weight 0.125
    float3 g3 = karis_average(h, i, k, m);      // bottom-left  weight 0.125
    float3 g4 = karis_average(i, j, l, n);      // bottom-right weight 0.125

    float3 result = g0 * 0.5 + (g1 + g2 + g3 + g4) * 0.125;
    output[dtid.xy] = result;
}
