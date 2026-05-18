// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)
struct Push_Constants {
    uint  input_id;
    uint  output_id;
    uint  bilinear_clamp_id;
    float radius_scale;
};
PUSH_CONSTANTS(Push_Constants, push);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID) 
{
    Texture2D   <float3> input  = ResourceDescriptorHeap[push.input_id];
    RWTexture2D <float3> output = ResourceDescriptorHeap[push.output_id];
    SamplerState bilinear_clamp = SamplerDescriptorHeap[push.bilinear_clamp_id];

    uint2 output_size;
    output.GetDimensions(output_size.x, output_size.y);

    if (any(dtid.xy >= output_size)) return;

    uint2 input_size;
    input.GetDimensions(input_size.x, input_size.y);

    float2 uv = (float2(dtid.xy) + 0.5f) / float2(output_size);
    // @Todo: Parameterize radius
    float2 d = 1.0f / float2(input_size) * push.radius_scale;

    float3 result = 0.0f;

    // 3x3 Tent Filter
    static const float w1 = 1.0f / 16.0f;
    static const float w2 = 2.0f / 16.0f;
    static const float w4 = 4.0f / 16.0f;

    result += input.Sample(bilinear_clamp, uv + float2(-d.x, -d.y)) * w1;
    result += input.Sample(bilinear_clamp, uv + float2(0.0f, -d.y)) * w2;
    result += input.Sample(bilinear_clamp, uv + float2( d.x, -d.y)) * w1;

    result += input.Sample(bilinear_clamp, uv + float2(-d.x, 0.0f)) * w2;
    result += input.Sample(bilinear_clamp, uv + float2(0.0f, 0.0f)) * w4;
    result += input.Sample(bilinear_clamp, uv + float2( d.x, 0.0f)) * w2;

    result += input.Sample(bilinear_clamp, uv + float2(-d.x,  d.y)) * w1;
    result += input.Sample(bilinear_clamp, uv + float2(0.0f,  d.y)) * w2;
    result += input.Sample(bilinear_clamp, uv + float2( d.x,  d.y)) * w1;

    output[dtid.xy] += result;
}
