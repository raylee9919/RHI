// Copyright Seong Woo Lee. All Rights Reserved.


#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Material
{
    int albedo_id;
    int normal_id;
    int orm_id;
    int emissive_id;
    int sampler_id;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
};

struct Camera
{
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;

    float4 position;
};

struct PushConstants
{
    uint vertex_buffer_id;
    uint material_id;
    uint camera_id;
};
PUSH_CONSTANTS(PushConstants, push);

struct PS_Input
{
    float4 sv_position : SV_POSITION;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 uv          : UV;
    float4 tangent     : TANGENT;
};

PS_Input VS_Main(uint vertex_id : SV_VertexID)
{
    PS_Input result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    ConstantBuffer<Camera> camera = ResourceDescriptorHeap[push.camera_id];

    float4 position = float4(vert.position, 1.0);
    float4x4 view_proj = camera.view_proj;
    float4 clip_space_position = mul(view_proj, position);
    result.sv_position = clip_space_position;
    result.position    = vert.position;
    result.normal      = vert.normal;
    result.uv          = vert.uv;
    result.tangent     = vert.tangent;

    return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    StructuredBuffer<Material> material_buf = ResourceDescriptorHeap[push.material_id];
    Material material = material_buf[0];

    SamplerState sampler_linear = SamplerDescriptorHeap[material.sampler_id];
    Texture2D albedo_tex = ResourceDescriptorHeap[material.albedo_id];
    float4 albedo = albedo_tex.Sample(sampler_linear, input.uv);

    Texture2D normal_tex = ResourceDescriptorHeap[material.normal_id];
    float3 normal = normal_tex.Sample(sampler_linear, input.uv).xyz;

    return albedo;
}
