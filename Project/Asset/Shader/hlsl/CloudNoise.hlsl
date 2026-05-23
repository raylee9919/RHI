// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Push_Constants {
    uint texture_id;
};
PUSH_CONSTANTS(Push_Constants, push);

[numthreads(4, 4, 2)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture3D<float4> tex = ResourceDescriptorHeap[push.texture_id];
    uint3 dim;
    tex.GetDimensions(dim.x, dim.y, dim.z);
    if (any(dtid >= dim)) return;


    float v = 1.0;
    tex[dtid] = float4(v, v, v, 1.0);
}
