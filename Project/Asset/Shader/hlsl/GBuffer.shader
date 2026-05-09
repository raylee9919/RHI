// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float4 tangent     : TANGENT;
    float2 uv          : UV;
};

struct PS_Output {
    float4 position  : SV_TARGET0;
    float4 normal    : SV_TARGET1;
    float2 uv        : SV_TARGET2;
    uint material_id : SV_TARGET3;
};

// @Todo: I feel like separating positions from attributes is better.
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
    uint anisotropic_sampler_id;
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
    result.normal      = vert.normal;
    result.tangent     = vert.tangent;
    result.uv          = vert.uv;

    return result;
}

PS_Output ps_main(PS_Input input)
{
    // material.
    StructuredBuffer <Material> material_buffer = ResourceDescriptorHeap[push.material_id];
    Material material = material_buffer[0];

    // sampler.
    SamplerState anisotropic_sampler = SamplerDescriptorHeap[push.anisotropic_sampler_id];

    // TBN matrix.
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = float3x3(T, B, N);

    // Compute normal.
    float3 n;
    uint normal_id = material.normal_id;
    if (normal_id == 0xffffffff) {
        n = input.normal;
    } else {
        Texture2D normal_tex = ResourceDescriptorHeap[normal_id];
        float3 normal_texel = normal_tex.Sample(anisotropic_sampler, input.uv).xyz * 2.0 - 1.0;
        n = mul(normal_texel, TBN);
    }

    PS_Output result;
    {
        result.position    = float4(input.position, 1.0);
        result.normal      = float4(n, 0.0);
        result.uv          = input.uv;
        result.material_id = push.material_id;
    }
    return result;
}
