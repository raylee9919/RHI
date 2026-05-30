// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)
struct Push_Constants {
    uint  color_id;
    uint  bilinear_clamp_id;
    uint  bloom_id;
    float bloom_strength;
};
PUSH_CONSTANTS(Push_Constants, push);

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float2 uv          : TEXCOORD0;
};

PS_Input vs_main(uint vertex_id : SV_VertexID)
{
    PS_Input result;
    float2 pos  = float2((vertex_id & 1) ? 3.0 : -1.0,
                         (vertex_id & 2) ? -3.0 : 1.0);
    result.sv_position = float4(pos, 0.0, 1.0);
    result.uv          = float2((vertex_id & 1) ? 2.0 : 0.0,
                                (vertex_id & 2) ? 2.0 : 0.0);
    return result;
}

float3 ps_main(PS_Input input) : SV_TARGET
{
    SamplerState bilinear_clamp = SamplerDescriptorHeap[push.bilinear_clamp_id];

    Texture2D <float3> color_tex = ResourceDescriptorHeap[push.color_id];
    Texture2D <float3> bloom_tex = ResourceDescriptorHeap[push.bloom_id];

    float3 color = color_tex.Sample(bilinear_clamp, input.uv);
    float3 bloom = bloom_tex.Sample(bilinear_clamp, input.uv);

    float3 result = color + (bloom * push.bloom_strength);
    return result;
}
