// Copyright Seong Woo Lee. All Rights Reserved.

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

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

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float2 screen_uv   : TEXCOORD0;
};

struct Push_Constants {
    uint position_id;
    uint normal_id;
    uint uv_id;
    uint material_id;
    uint camera_id;
    uint linear_sampler_id;
    uint anisotropic_sampler_id;
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
    // g-buffer.
    Texture2D <float4> gbuffer_position  = ResourceDescriptorHeap[push.position_id];
    Texture2D <float2> gbuffer_uv        = ResourceDescriptorHeap[push.uv_id];
    Texture2D <uint> gbuffer_material_id = ResourceDescriptorHeap[push.material_id];
    
    // Camera.
    StructuredBuffer <Camera> camera_buffer = ResourceDescriptorHeap[push.camera_id];
    Camera camera = camera_buffer[0];

    // sampler.
    SamplerState linear_sampler      = SamplerDescriptorHeap[push.linear_sampler_id];
    SamplerState anisotropic_sampler = SamplerDescriptorHeap[push.anisotropic_sampler_id];

    // position
    float3 position = gbuffer_position.Sample(linear_sampler, input.screen_uv).xyz;

    // uv
    float2 uv = gbuffer_uv.Sample(linear_sampler, input.screen_uv).xy;

    // material.
    uint material_id = gbuffer_material_id.Load(int3(input.sv_position.xy, 0));
    StructuredBuffer <Material> material_buffer = ResourceDescriptorHeap[NonUniformResourceIndex(material_id)];
    Material material = material_buffer[0];

    // albedo.
    Texture2D <float4> albedo_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.albedo_id)];
    float3 albedo = albedo_texture.Sample(anisotropic_sampler, uv).xyz;

    // normal.
    Texture2D <float4> normal_texture = ResourceDescriptorHeap[NonUniformResourceIndex(push.normal_id)];
    float3 n = normal_texture.Sample(anisotropic_sampler, input.screen_uv).xyz;

    // @Temporary: Debug light scene
    float3 radiance = 1.0;
    float3 l = normalize(float3(1.0, 1.0, -1.0));
    float3 v = normalize(camera.position.xyz - position);
    float cos_falloff = max(dot(n, l), 0.0);
    float3 res = radiance * cos_falloff * albedo;

    float4 result = float4(res, 1.0);
    return result;
}
