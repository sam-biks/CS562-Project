// These definitions agree with the ObjectIds enum in scene.h
#include "ShaderData.h"
static const int nullId = 0;
static const int skyId = 1;
static const int seaId = 2;
static const int groundId = 3;
static const int roomId = 4;
static const int boxId = 5;
static const int frameId = 6;
static const int lPicId = 7;
static const int rPicId = 8;
static const int teapotId = 9;
static const int spheresId = 10;
static const int floorId = 11;

static const float pi = 3.14159;
static const float pi2 = 2 * pi;

Texture2D ObjectTexture : register(t0);
SamplerState StaticSampler : register(s0);
struct PixelIn
{
    float4 position : SV_Position;
    float3 normalVec : NORMAL;
    float4 worldPosition : WORLDPOS;
    float2 texCoord : TEXCOORD;
};

struct PixelOut
{
    float4 worldPosition : SV_Target0;
    float4 normal : SV_Target1;
    float4 diffuse : SV_Target2;
    float4 specularAlpha : SV_Target3;
};

float2 UVOF(float3 w)
{
    //float u = atan2(w.x, -w.z) / pi2 + 0.5f;
    //return float2(u, acos(w.y) / pi);
    return float2(0.5f - atan2(w.y, w.x) / pi2, acos(w.z) / pi);
}

PixelOut main(PixelIn _input)
{
    PixelOut output;
    output.worldPosition = _input.worldPosition;
    output.normal = float4(_input.normalVec, 0);
    if (Textured)
    {
        //output.diffuse = ObjectTexture.SampleLevel(StaticSampler, UVOF(float3(_input.normalVec.x, _input.normalVec.z, -_input.normalVec.y)), 0);
        output.diffuse = ObjectTexture.SampleLevel(StaticSampler, _input.texCoord, 0);
    }
    else
        output.diffuse = float4(diffuse, 0);
    output.specularAlpha = float4(specular, roughness);
    
    return output;
}