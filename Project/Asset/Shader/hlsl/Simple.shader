// Copyright Seong Woo Lee. All Rights Reserved.


#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Material {
    int albedo_id;
    int normal_id;
    int orm_id;
    int emissive_id;
    int sampler_id;
};

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
    float4 position;
};

struct Push_Constants {
    uint vertex_buffer_id;
    uint material_id;
    uint camera_id;
};
PUSH_CONSTANTS(Push_Constants, push);

struct PS_Input {
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

    ConstantBuffer<Camera> camera = ResourceDescriptorHeap[push.camera_id];

    SamplerState sampler_linear = SamplerDescriptorHeap[material.sampler_id];
    Texture2D albedo_tex = ResourceDescriptorHeap[material.albedo_id];
    float4 albedo = albedo_tex.Sample(sampler_linear, input.uv);

    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz);
    float3 B = cross(N, T) * input.tangent.w;
    float3x3 TBN = float3x3(T, B, N);

    // Compute normal.
    //
    float3 n;
    int normal_id = material.normal_id;
    if (normal_id == -1) {
        n = input.normal;
    } else {
        Texture2D normal_tex = ResourceDescriptorHeap[normal_id];
        float3 normal_texel = normal_tex.Sample(sampler_linear, input.uv).xyz * 2.0 - 1.0;
        n = mul(normal_texel, TBN);
    }


    // @Temporary: Debug light scene
    //
    float3 radiance = float3(1.5, 1.5, 1.5);
    float3 l = normalize(float3(1.0, 2.0, 0.0));
    float3 v = normalize(camera.position.xyz - input.position);

    float attenuation = max(dot(n, l), 0.0);

    float3 result = attenuation * albedo.xyz;

    return float4(result, 1.0);
}
