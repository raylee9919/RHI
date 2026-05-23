// Copyright Seong Woo Lee. All Rights Reserved.

#define PI              3.14159265
#define NDC_DEPTH_MAX   1.0
static const int    NUM_SAMPLES         = 16;
static const int    NUM_SAMPLES_TO_SUN  = 6;

static const float  PLANET_RADIUS       = 6360e3;
static const float  ATMOSPHERE_RADIUS   = 6420e3;
static const float  INNER_RADIUS        = PLANET_RADIUS + 1.5e3;
static const float  OUTER_RADIUS        = PLANET_RADIUS + 4.5e3;
static const float3 PLANET_CENTER       = float3(0.0, -PLANET_RADIUS, 0.0);

static const float3 SKY_RADIANCE        = float3(0.2, 0.2, 1.0) * 20.0;
static const float3 SUN_RADIANCE        = float3(1.0, 1.0, 1.0) * 20.0;
static const float3 TO_SUN              = normalize(float3(1.0, 1.0, 1.0));

struct Camera {
    float4   position;
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float4x4 inv_view;
    float4x4 inv_proj;
};

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)
struct Push_Constants {
    uint camera_id;
    
    uint noise_id;
    uint weather_map_id;

    uint linear_wrap_id;
};
PUSH_CONSTANTS(Push_Constants, push);

struct PS_Input {
    float4 sv_position : SV_POSITION;
    float2 screen_uv   : TEXCOORD0;
};

bool ray_sphere(float3 ray_origin, float3 ray_dir, 
                float3 center, float radius, 
                out float tmin, out float tmax)
{
    float3 oc = ray_origin - center;

    float a = dot(ray_dir, ray_dir);
    float b = dot(oc, ray_dir);
    float c = dot(oc, oc) - radius * radius;

    float discriminant = b * b - a * c;

    if (discriminant < 0.0f)
    {
        tmin = tmax = 0.0f;
        return false;
    }

    float sqrtD = sqrt(discriminant);
    tmin = (-b - sqrtD) / a;
    tmax = (-b + sqrtD) / a;

    return tmax >= 0.0f;
}

float isotropic_phase_function() {
    return 0.07957747154; // = 1/4PI
}

float eval_density(float3 position, SamplerState linear_wrap, 
                   Texture2D<float3> weather_map, Texture3D<float4> noise_tex) 
{
    float uv_scale = 1e-3; // @Temporary
    float density = noise_tex.Sample(linear_wrap, float3(position.zxy * uv_scale), 0).x;
    density = smoothstep(0.3, 0.8, density);
    // @Temporary
    //float3 weather = weather_map.Sample(linear_wrap, float2(position.zx * 1.666666e-5)); // 60km^2
    //float coverage = weather.x;
    // @Temporary
    //coverage = smoothstep(0.3, 0.9, coverage);
    //density *= coverage;

    return density;
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

float3 ps_main(PS_Input input) : SV_TARGET
{
    // Pull camera.
    StructuredBuffer <Camera> camera_buffer = ResourceDescriptorHeap[push.camera_id];
    Camera camera = camera_buffer[0];
    
    // Pull sampler and textures.
    SamplerState linear_wrap = SamplerDescriptorHeap[push.linear_wrap_id];
    Texture3D <float4> noise_tex   = ResourceDescriptorHeap[push.noise_id];
    Texture2D <float3> weather_map = ResourceDescriptorHeap[push.weather_map_id];

    // 
    float3 camera_position = camera.position.xyz;
    float ndc_x =  2.0 * input.screen_uv.x - 1.0;
    float ndc_y = -2.0 * input.screen_uv.y + 1.0;
    float4 ndc = float4(ndc_x, ndc_y, NDC_DEPTH_MAX, 1.0);
    float4 view_space = mul(camera.inv_proj, ndc);
    view_space /= view_space.w;

    // Reconstruct ray
    float3 view_dir = normalize(mul(camera.inv_view, float4(view_space.xyz, 0.0))).xyz;

    float tmin_in, tmax_in;
    ray_sphere(camera_position, view_dir, PLANET_CENTER, INNER_RADIUS, tmin_in, tmax_in);
    float3 hit_pos_in = camera_position + view_dir * tmax_in;

    float tmin_out, tmax_out;
    ray_sphere(camera_position, view_dir, PLANET_CENTER, OUTER_RADIUS, tmin_out, tmax_out);
    float3 hit_pos_out = camera_position + view_dir * tmax_out;

    // Coefficients
    float scale = 1e-3f;
    float absorption = 0.2 * scale;
    float scattering = 0.8 * scale;
    float extinction = absorption + scattering;

    // Ray march
    float delta = length(hit_pos_out - hit_pos_in);
    float step_size = delta / (NUM_SAMPLES + 1);
    float3 sample_position = hit_pos_in;

    // @Temporary
    if (hit_pos_in.y < 0.0) return 0.0;

    float transmittance = 1.0;
    float3 radiance_from_sun = 0.0;

    [loop]
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sample_position += (step_size * view_dir);

        float density = eval_density(sample_position, linear_wrap, weather_map, noise_tex);

        return density;

        float attenuation = exp(-density * extinction * step_size);
        transmittance *= attenuation;

        // Ray march towards sun
        float tmin_sun, tmax_sun;
        ray_sphere(sample_position, TO_SUN, PLANET_CENTER, OUTER_RADIUS, tmin_sun, tmax_sun);
        float3 hit_pos_sun = sample_position + TO_SUN * tmax_sun;
        float step_to_sun = distance(hit_pos_sun, sample_position) / (NUM_SAMPLES_TO_SUN + 1);
        float tau = 0.0;

        float3 sample_position_sun = sample_position;
        for (int j = 0; j < NUM_SAMPLES_TO_SUN; ++j) {
            sample_position_sun += (step_to_sun * TO_SUN);
            float density_sun = eval_density(sample_position_sun, linear_wrap, weather_map, noise_tex);
            tau += density_sun;
        }
        float attenuation_sun = exp(-tau * step_to_sun * extinction);
        radiance_from_sun += (SUN_RADIANCE * attenuation_sun * 
                             isotropic_phase_function() * scattering * 
                             transmittance * step_size * density);
    }

    float3 result = (transmittance * SKY_RADIANCE) + radiance_from_sun;
    return result;
}
