// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Push_Constants {
    uint input_id;
    uint output_id;
    uint bilinear_clamp_id;
};
PUSH_CONSTANTS(Push_Constants, push);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D <float4> input  = ResourceDescriptorHeap[push.input_id];
    RWTexture2D <float4> output = ResourceDescriptorHeap[push.output_id];
    SamplerState bilinear_clamp = SamplerDescriptorHeap[push.bilinear_clamp_id];

    uint2 out_dims;
    output.GetDimensions(out_dims.x, out_dims.y);

    if (any(dtid.xy >= out_dims))
        return;

    uint2 in_dims;
    input.GetDimensions(in_dims.x, in_dims.y);

    // Texel size of the input
    float2 texel = 1.0 / float2(in_dims);

    // UV of the center of the output pixel, mapped into input space
    // Output is half-res of input, so each output pixel covers a 2x2 input block
    float2 uv = (float2(dtid.xy) + 0.5) * texel * 2.0;

    // 13-tap filter from "Next Generation Post Processing in Call of Duty: Advanced Warfare"
    // (Jorge Jimenez, SIGGRAPH 2014)
    //
    //  [a] . [b] . [c]
    //  . [e] . [f] .
    //  [g] . [h] . [i]
    //  . [j] . [k] .
    //  [l] . [m] . [n]
    //
    // 4 bilinear samples at half-offset corners (e,f,j,k) each cover a 2x2 block → weight 0.5 total
    // 4 bilinear samples at full-offset corners (a,c,l,n)                          → weight 0.125 total
    // 4 edge bilinear samples (b,g,i,m)                                            → weight 0.25 total (shared)
    // 1 center sample (h)                                                           → weight 0.125

    float4 a = input.SampleLevel(bilinear_clamp, uv + float2(-2, -2) * texel, 0);
    float4 b = input.SampleLevel(bilinear_clamp, uv + float2( 0, -2) * texel, 0);
    float4 c = input.SampleLevel(bilinear_clamp, uv + float2( 2, -2) * texel, 0);

    float4 g = input.SampleLevel(bilinear_clamp, uv + float2(-2,  0) * texel, 0);
    float4 h = input.SampleLevel(bilinear_clamp, uv + float2( 0,  0) * texel, 0);
    float4 i = input.SampleLevel(bilinear_clamp, uv + float2( 2,  0) * texel, 0);

    float4 l = input.SampleLevel(bilinear_clamp, uv + float2(-2,  2) * texel, 0);
    float4 m = input.SampleLevel(bilinear_clamp, uv + float2( 0,  2) * texel, 0);
    float4 n = input.SampleLevel(bilinear_clamp, uv + float2( 2,  2) * texel, 0);

    float4 e = input.SampleLevel(bilinear_clamp, uv + float2(-1, -1) * texel, 0);
    float4 f = input.SampleLevel(bilinear_clamp, uv + float2( 1, -1) * texel, 0);
    float4 j = input.SampleLevel(bilinear_clamp, uv + float2(-1,  1) * texel, 0);
    float4 k = input.SampleLevel(bilinear_clamp, uv + float2( 1,  1) * texel, 0);

    float4 result =
        0.125     * h                       +  // center
        0.03125   * (a + c + l + n)         +  // far corners  (0.125 / 4)
        0.0625    * (b + g + i + m)         +  // edges        (0.25  / 4)
        0.125     * (e + f + j + k);           // inner quad   (0.5   / 4)

    output[dtid.xy] = result;
}
