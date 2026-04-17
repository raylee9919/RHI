// Copyright Seong Woo Lee. All Rights Reserved.


#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Camera
{
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;

    float4 position;
};
//=============================================================================

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
};

struct PushConstants
{
    uint vertex_buffer_id;
    uint texture_id;
    uint sampler_id;

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
    result.sv_position = mul(camera.view_proj, position);
    result.position    = vert.position;
    result.normal      = vert.normal;
    result.uv          = vert.uv;
    result.tangent     = vert.tangent;

    return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    Texture2D<float4> tex = ResourceDescriptorHeap[push.texture_id];
    SamplerState sam = SamplerDescriptorHeap[push.sampler_id];

    float3 light_position = float3(1.0, 1.0, 1.0);
    float3 light_radiance = float3(1.0, 1.0, 1.0);

    ConstantBuffer<Camera> camera = ResourceDescriptorHeap[push.camera_id];

    float3 l = normalize(light_position - input.position);
    float3 v = normalize(camera.position.xyz - input.position);
    float3 h = normalize(l + v);


    float4 albedo = tex.Sample(sam, input.uv);

    float4 result = float4(input.normal, 1.0);

    return result;
}
