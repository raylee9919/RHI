// Copyright Seong Woo Lee. All Rights Reserved.

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 tangent;
};

struct BindlessConstants
{
    uint vertex_buffer_index;
    uint texture_index;
    uint sampler_index;
};
ConstantBuffer<BindlessConstants> bindless : register(b0);

struct PS_Input
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : UV;
    float4 tangent  : TANGENT;
};

PS_Input VS_Main(uint vertex_id : SV_VertexID)
{
    PS_Input result;

    StructuredBuffer<Vertex> vertex_buffer = ResourceDescriptorHeap[bindless.vertex_buffer_index];
    Vertex vert = vertex_buffer[vertex_id];

    result.position = float4(vert.position, 1.0);
    result.uv = vert.uv;
    result.normal = vert.normal;
    result.uv = vert.uv;
    result.tangent = vert.tangent;

    return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    Texture2D<float4> tex = ResourceDescriptorHeap[bindless.texture_index];
    SamplerState sam = SamplerDescriptorHeap[bindless.sampler_index];

    float4 result = tex.Sample(sam, input.uv);
    return result;
}
