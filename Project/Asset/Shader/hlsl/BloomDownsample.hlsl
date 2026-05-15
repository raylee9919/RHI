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
    int2 uv = dtid.xy;
    RWTexture2D <float4> input  = ResourceDescriptorHeap[push.input_id];
    RWTexture2D <float4> output = ResourceDescriptorHeap[push.output_id];
    SamplerState bilinear_clamp = SamplerDescriptorHeap[push.bilinear_clamp_id];
}
