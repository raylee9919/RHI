// Copyright Seong Woo Lee. All Rights Reserved.

#define PI 3.14159265
#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float2 screen_uv   : TEXCOORD0;
};

struct Point_Light {
    float3 position;
    float3 radiance;
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
    uint position_id;
    uint normal_id;
    uint uv_id;
    uint material_id;
    uint camera_id;
    uint linear_sampler_id;
    uint anisotropic_sampler_id;
};
PUSH_CONSTANTS(Push_Constants, push);

float trowbridge_reitz_ggx(float roughness, float ndoth) {
    float a = roughness;
    float a2 = roughness * roughness;
    float denom = ndoth * ndoth * (a2 - 1) + 1;
    return a2 / (PI * denom * denom);
}

float3 schlick_fresnel(float3 f0, float vdoth) {
    // Spherical Gaussian appoximation to replace the power.
    // F0 is the specular reflectance at normal incidence.
    return f0 + (1 - f0) * pow(2.0, (-5.55473 * vdoth - 6.98316) * vdoth);
}

float smith_g1(float c, float roughness) {
    float r = roughness + 1;
    float k = (r * r) / 8.0;
    return c / (c * (1 - k) + k);
}

float smith_ggx(float ndotl, float ndotv, float roughness) {
    return smith_g1(ndotl, roughness) * smith_g1(ndotv, roughness);
}

float3 compute_irradiance(Point_Light light, Camera camera, float3 position, float3 normal,
                          float3 albedo, float roughness, float metallic) {
    float3 n = normal; // alias
    float3 l = normalize(light.position - position);
    float3 v = normalize(camera.position.xyz - position);
    float3 h = normalize(l + v);
    float ndotl = max(dot(n, l), 0.0001);
    float ndotv = max(dot(n, v), 0.0001);
    float ndoth = dot(n, h);
    float vdoth = dot(v, h);
    
    float3 f0 = lerp(0.04, albedo, metallic);

    float  D = trowbridge_reitz_ggx(roughness, ndoth);
    float3 F = schlick_fresnel(f0, vdoth);
    float  G = smith_ggx(ndotl, ndotv, roughness);
    float3 kd = (1.0 - F) * (1.0 - metallic);
    float3 diffuse = kd * albedo / PI;
    float3 BRDF = diffuse + (D * F * G) / (4.0 * ndotl * ndotv);

    float dist = distance(light.position, position);
    float dist_falloff = 1.0 / (dist * dist);

    float3 result = BRDF * ndotl * light.radiance * dist_falloff;
    return result;
}

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
    // g-buffer
    Texture2D <float4> gbuffer_position  = ResourceDescriptorHeap[push.position_id];
    Texture2D <float2> gbuffer_uv        = ResourceDescriptorHeap[push.uv_id];
    Texture2D <uint> gbuffer_material_id = ResourceDescriptorHeap[push.material_id];
    
    // Camera
    StructuredBuffer <Camera> camera_buffer = ResourceDescriptorHeap[push.camera_id];
    Camera camera = camera_buffer[0];

    // sampler
    SamplerState linear_sampler      = SamplerDescriptorHeap[push.linear_sampler_id];
    SamplerState anisotropic_sampler = SamplerDescriptorHeap[push.anisotropic_sampler_id];

    // position
    float3 position = gbuffer_position.Sample(linear_sampler, input.screen_uv).xyz;

    // normal
    Texture2D <float4> normal_texture = ResourceDescriptorHeap[NonUniformResourceIndex(push.normal_id)];
    float3 n = normal_texture.Sample(anisotropic_sampler, input.screen_uv).xyz;

    // uv
    float2 uv = gbuffer_uv.Sample(linear_sampler, input.screen_uv).xy;

    // material
    uint material_id = gbuffer_material_id.Load(int3(input.sv_position.xy, 0));
    StructuredBuffer <Material> material_buffer = ResourceDescriptorHeap[NonUniformResourceIndex(material_id)];
    Material material = material_buffer[0];

    // albedo
    Texture2D <float4> albedo_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.albedo_id)];
    float3 albedo = albedo_texture.Sample(anisotropic_sampler, uv).xyz;

    // orm
    float occlusion = 0.0;
    float roughness = 1.0;
    float metallic  = 0.0;
    uint orm_id = material.orm_id;
    if (orm_id != 0xffffffff) {
        Texture2D <float4> orm_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.orm_id)];
        float3 orm = orm_texture.Sample(anisotropic_sampler, uv).xyz;
        occlusion = orm.x;
        roughness = orm.y;
        metallic  = orm.z;
    }

    // @Temporary: Debug light scene
    Point_Light lights[2];
    lights[0].position = float3(-200.0, 500.0, 0.0);
    lights[0].radiance = 300000.0;
    lights[1].position = float3( 500.0, 800.0, 0.0);
    lights[1].radiance = float3(100000.0, 100000.0, 300000.0);

    float3 result = 0.0;

    [unroll]
    for (int i = 0; i < 2; ++i) {
        result += compute_irradiance(lights[i], camera, position, n, albedo, roughness, metallic);
    }

    // @Todo: Correct gamma correction
    result = pow(result, 1.0 / 2.2);

    return float4(result, 1.0);
}
