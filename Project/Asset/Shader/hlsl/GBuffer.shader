// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float3 position    : POSITION;
};

struct PS_Output {
    float3 position  : SV_TARGET0;
    uint material_id : SV_TARGET1;
};

// @Todo: I feel like separating positions from attributes is better.
struct Vertex {
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
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


PS_Input vs_main(uint vertex_id : SV_VertexID)
{
    PS_Input result;

    StructuredBuffer <Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    StructuredBuffer <Camera> camera_buffer = ResourceDescriptorHeap[push.camera_id];
    Camera camera = camera_buffer[0];

    result.sv_position = mul(camera.view_proj, float4(vert.position, 1.0));
    result.position    = vert.position;

    return result;
}

PS_Output ps_main(PS_Input input)
{
    PS_Output result;
    result.position    = float4(input.position, 1.0);
    result.material_id = push.material_id;
    return result;
}
