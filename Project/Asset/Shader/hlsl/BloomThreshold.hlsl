// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)
struct Push_Constants {
    uint  input_id;
    uint  output_id;
    uint  bilinear_clamp_id;
    float threshold;
};
PUSH_CONSTANTS(Push_Constants, push);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D   <float3> input          = ResourceDescriptorHeap[push.input_id];
    RWTexture2D <float3> output         = ResourceDescriptorHeap[push.output_id];
    SamplerState         bilinear_clamp = SamplerDescriptorHeap[push.bilinear_clamp_id];

    uint2 output_size;
    output.GetDimensions(output_size.x, output_size.y);

    float2 uv = (float2(dtid.xy) + 0.5) / float2(output_size);
    float3 texel = input.Sample(bilinear_clamp, uv);
    // @Todo: Correctness?
    float luminance = dot(texel, float3(0.2126, 0.7152, 0.0722));
    float3 result = step(push.threshold, luminance) * texel;
    output[dtid.xy] = result;
}
