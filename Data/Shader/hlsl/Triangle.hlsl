// Copyright Seong Woo Lee. All Rights Reserved.

struct PS_Input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PS_Input VS_Main(float4 position : POSITION, float4 color : COLOR)
{
    PS_Input result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    return input.color;
}
