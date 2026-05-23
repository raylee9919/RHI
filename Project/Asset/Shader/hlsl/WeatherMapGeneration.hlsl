// Copyright Seong Woo Lee. All Rights Reserved.

// WeatherMapGeneration.hlsl

#define PUSH_CONSTANTS(Type, Name) ConstantBuffer<Type> Name : register(b0)

struct Push_Constants {
    uint texture_id;
};
PUSH_CONSTANTS(Push_Constants, push);

// ─── Permutation table ────────────────────────────────────────────────────────
static const int P[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    // mirror
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
};

// ─── Core Perlin helpers ──────────────────────────────────────────────────────
float fade(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = (h < 4) ? x : y;
    float v = (h < 4) ? y : x;
    return (((h & 1) != 0) ? -u : u) + (((h & 2) != 0) ? -v : v);
}

float perlin2d(float2 p) {
    int xi = (int)floor(p.x) & 255;
    int yi = (int)floor(p.y) & 255;
    float xf = p.x - floor(p.x);
    float yf = p.y - floor(p.y);

    float u = fade(xf);
    float v = fade(yf);

    int aa = P[P[xi    ] + yi    ];
    int ab = P[P[xi    ] + yi + 1];
    int ba = P[P[xi + 1] + yi    ];
    int bb = P[P[xi + 1] + yi + 1];

    float r = lerp(
        lerp(grad(aa, xf,     yf    ), grad(ba, xf - 1.0, yf    ), u),
        lerp(grad(ab, xf,     yf - 1.0), grad(bb, xf - 1.0, yf - 1.0), u),
        v
    );
    return r * 0.5 + 0.5; // remap [-1,1] → [0,1]
}

// ─── Fractal Brownian Motion ──────────────────────────────────────────────────
struct FBM_Params {
    float2 offset;
    float  frequency;
    float  amplitude;
    float  lacunarity;  // frequency multiplier per octave (typically 2.0)
    float  persistence; // amplitude multiplier per octave (typically 0.5)
    int    octaves;
};

float fbm(float2 p, FBM_Params params) {
    float value = 0.0;
    float amp   = params.amplitude;
    float freq  = params.frequency;

    [unroll(8)]
    for (int i = 0; i < params.octaves; ++i) {
        value += perlin2d(p * freq + params.offset) * amp;
        amp   *= params.persistence;
        freq  *= params.lacunarity;
    }
    return saturate(value);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float3> tex = ResourceDescriptorHeap[push.texture_id];

    uint2 dim;
    tex.GetDimensions(dim.x, dim.y);
    if (any(dtid.xy >= dim)) return;

    float2 uv = (float2(dtid.xy) + 0.5) / float2(dim);

    // Cloud density  (R) — large low-frequency coverage
    FBM_Params density_params = {
        float2(1.7, 9.2),   // offset
        3.0,                // frequency
        0.85,               // amplitude
        2.0,                // lacunarity
        0.5,                // persistence
        6                   // octaves
    };

    // Cloud type     (G) — slow large-scale variation
    FBM_Params type_params = {
        float2(4.1, 2.8),
        1.0,
        1.0,
        2.0,
        0.5,
        4
    };

    // Wetness        (B) — fine high-frequency detail
    FBM_Params wet_params = {
        float2(8.3, 5.6),
        6.0,
        0.9,
        2.1,
        0.45,
        5
    };

    float r = fbm(uv, density_params);
    float g = fbm(uv, type_params);
    float b = fbm(uv, wet_params);

    tex[dtid.xy] = float3(r, g, b);
}
