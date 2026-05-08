// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 uv          : UV;
    float4 tangent     : TANGENT;
};

struct Vertex {
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
};

struct Material {
    uint albedo_id;
    uint normal_id;
    uint orm_id;
    uint emissive_id;
};

struct Camera {
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float4   position;
};

struct Push_Constants {
    uint vertex_buffer_id;
    uint material_id;
    uint camera_id;
};
PUSH_CONSTANTS(Push_Constants, push);


PS_Input VS_Main(uint vertex_id : SV_VertexID)
{
    PS_Input result;

    StructuredBuffer <Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    //StructuredBuffer <Material> material = ResourceDescriptorHeap[push.material_id];

    StructuredBuffer <Camera> camera_buffer = ResourceDescriptorHeap[push.camera_id];
    Camera camera = camera_buffer[0];

    result.sv_position = mul(camera.view_proj, float4(vert.position, 1.0));
    result.position    = vert.position;
    result.normal      = vert.normal;
    result.uv          = vert.uv;
    result.tangent     = vert.tangent;

    return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    return float4(input.tangent.xyz, 1.0);
}
