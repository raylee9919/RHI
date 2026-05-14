// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Push_Constants {
    uint texture_id;
};
PUSH_CONSTANTS(Push_Constants, push);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D <float4> tex = ResourceDescriptorHeap[push.texture_id];

    int2 coords = dtid.xy;

    float4 color = tex[coords];
    float luma = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    float4 result = float4(luma, luma, luma, 1.0);

    tex[coords] = result;
}
