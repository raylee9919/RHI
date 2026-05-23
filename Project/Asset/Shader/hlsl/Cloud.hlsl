// Copyright Seong Woo Lee. All Rights Reserved.

#define PI              3.14159265
#define NDC_DEPTH_MAX   1.0
static const float  PLANET_RADIUS       = 6360e3;
static const float  ATMOSPHERE_RADIUS   = 6420e3;
static const float3 PLANET_CENTER       = float3(0.0, -PLANET_RADIUS, 0.0);
static const float3 TO_SUN              = normalize(float3(-1.0, 1.0, 0.0));
static const float3 SUN_RADIANCE        = float3(1.0, 1.0, 1.0) * 20;
static const int NUM_SAMPLES_VIEW       = 16;
static const int NUM_SAMPLES_SUN        = 8;

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

    if (discriminant < 0.0f) {
        tmin = tmax = 0.0f;
        return false;
    }

    float sqrtD = sqrt(discriminant);
    tmin = (-b - sqrtD) / a;
    tmax = (-b + sqrtD) / a;

    return tmax >= 0.0f;
}

float altitude_of(float3 position) {
    return distance(position, PLANET_CENTER) - PLANET_RADIUS;
}

float phase_rayleigh(float u) {
    return 0.05968310365 * (1 + u*u);
}

float phase_mie(float u, float g) {
    float g2 = g*g;
    return 0.11936620731 * (((1 - g2)*(1 + u*u)) / ((2 + g2) * pow(1 + g2 - 2*g*u, 1.5))); // @Todo: abs...?
}

PS_Input vs_main(uint vertex_id : SV_VertexID)
{
    PS_Input result;
    float2 pos  = float2((vertex_id & 1) ? 3.0 : -1.0, (vertex_id & 2) ? -3.0 : 1.0);
    result.sv_position = float4(pos, 0.0, 1.0);
    result.screen_uv   = float2((vertex_id & 1) ? 2.0 : 0.0, (vertex_id & 2) ? 2.0 : 0.0);
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
    float3 to_light = TO_SUN;
    float3 camera_position = camera.position.xyz;
    float ndc_x =  2.0 * input.screen_uv.x - 1.0;
    float ndc_y = -2.0 * input.screen_uv.y + 1.0;
    float4 ndc = float4(ndc_x, ndc_y, NDC_DEPTH_MAX, 1.0);
    float4 view_space = mul(camera.inv_proj, ndc);
    view_space /= view_space.w;

    // Reconstruct ray
    float3 view_dir = normalize(mul(camera.inv_view, float4(view_space.xyz, 0.0))).xyz;
    float tmin, tmax;
    if (!ray_sphere(camera_position, view_dir, PLANET_CENTER, ATMOSPHERE_RADIUS, tmin, tmax)) {
        return 0.0;
    }
    float dist = tmin < 0.0 ? tmax : tmin;

    float step_size = dist / float(NUM_SAMPLES_VIEW + 1);
    float3 dX       = step_size * view_dir;                 // Step
    float3 X        = camera_position + dX;                 // Sample position along view direction
    float3 T        = 1.0;                                  // Transmittance
    float3 L0       = SUN_RADIANCE;                         // Initial radiance
    float mu        = dot(view_dir, to_light);              // Cosine of view dir and light dir
    float3 Sr       = float3(5.8e-6, 1.35e-5, 3.31e-5);     // Rayleigh scattering
    float3 Sm       = 2.1e-5;                               // Mie scattering. @Todo: Change according to weather, pollution, etc.
    float Pr        = phase_rayleigh(mu);                   // Rayleigh phase function
    float Pm        = phase_mie(mu, 0.76);                  // Mie phase function
    float Dr        = 8e3;                                  // Rayleigh distribution
    float Dm        = 1.2e3;                                // Mie distribution
    float ODr       = 0.0;                                  // Optical depth
    float ODm       = 0.0;                                  // 
    float3 sum_r    = 0.0;
    float3 sum_m    = 0.0;

    [loop]
    for (int i = 0; i < NUM_SAMPLES_VIEW; ++i) {
        float h = altitude_of(X); // Altitude

        float Hr = exp(-h / Dr) * step_size;
        float Hm = exp(-h / Dm) * step_size;

        ODr += Hr;
        ODm += Hm;

        float t0, t1;
        ray_sphere(X, to_light, PLANET_CENTER, ATMOSPHERE_RADIUS, t0, t1);
        float dist_L      = t1; // @Todo: Outer earth
        float step_size_L = dist_L / float(NUM_SAMPLES_SUN + 1);
        float3 step_L     = step_size_L * to_light;
        float3 sample_L   = X + step_L;
        float ODr_L = 0;
        float ODm_L = 0;

        for (int j = 0; j < NUM_SAMPLES_SUN; ++j) {
            float h_L = altitude_of(sample_L);

            ODr_L += exp(-h_L / Dr) * step_size_L;
            ODm_L += exp(-h_L / Dm) * step_size_L;

            sample_L += step_L;
        }

        float3 tau = Sr * (ODr + ODr_L) + Sm * 1.11 * (ODm + ODm_L);
        float3 attenuation = exp(-tau);
        sum_r += attenuation * Hr;
        sum_m += attenuation * Hm;

        X += dX;
    }

    float3 L = L0 * ((sum_r * Sr * Pr) + (sum_m * Sm * Pm));
    return L;
}
