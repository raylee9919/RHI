// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Push_Constants {
    uint vertex_buffer_id;
};
PUSH_CONSTANTS(Push_Constants, push);

struct PS_Input {
    float4 sv_position : SV_POSITION;
};

struct Vertex {
    float3 position;
};

PS_Input VS_Main(uint vertex_id : SV_VertexID)
{
    PS_Input result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[push.vertex_buffer_id];
    Vertex vert = vertex_buffer[vertex_id];

    result.sv_position = float4(vert.position, 1.0);

    return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
