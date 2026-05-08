// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Camera {
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float4   position;
};

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float2 uv          : TEXCOORD0;
};

struct Push_Constants {
    uint gbuffer_position;
    uint gbuffer_material_id;
    uint camera_id;
};
PUSH_CONSTANTS(Push_Constants, push);

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

float4 ps_main(PS_Input input) : SV_TARGET
{
    // G-Buffer
    Texture2D <float4> gbuffer_position    = ResourceDescriptorHeap[push.gbuffer_position];
    Texture2D <uint>   gbuffer_material_id = ResourceDescriptorHeap[push.gbuffer_material_id];

    float3 world_pos    = gbuffer_position.Sample(sampler0, input.uv).xyz;
    uint   material_id  = gbuffer_material_id.Load(int3(input.sv_position.xy, 0));

    float4 result = float4(world_pos, 1.0);
    return result;
}
