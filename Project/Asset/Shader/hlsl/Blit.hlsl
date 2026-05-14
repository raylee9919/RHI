// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float2 screen_uv   : TEXCOORD0;
};

struct Push_Constants {
    uint color_id;
    uint linear_sampler_id;
};
PUSH_CONSTANTS(Push_Constants, push);

PS_Input vs_main(uint vertex_id : SV_VertexID)
{
    PS_Input result;
    float2 pos  = float2((vertex_id & 1) ? 3.0 : -1.0,
                         (vertex_id & 2) ? -3.0 : 1.0);
    result.sv_position = float4(pos, 0.0, 1.0);
    result.screen_uv   = float2((vertex_id & 1) ? 2.0 : 0.0,
                                (vertex_id & 2) ? 2.0 : 0.0);
    return result;
}

float4 ps_main(PS_Input input) : SV_TARGET
{
    // sampler.
    SamplerState linear_sampler = SamplerDescriptorHeap[push.linear_sampler_id];

    // g-buffer.
    Texture2D <float4> color_texture = ResourceDescriptorHeap[push.color_id];
    float4 color = color_texture.Sample(linear_sampler, input.screen_uv);

    float4 result = color;
    return result;
}
