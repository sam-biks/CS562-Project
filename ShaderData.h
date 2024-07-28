#pragma once
#ifndef __cplusplus
#define RootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "\
                "CBV(b0), CBV(b1), CBV(b2), DescriptorTable(SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
                "StaticSampler(s0, MinLOD=0, MaxLOD=3.402823466e+38f)"

#define LightingRootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "\
                        "CBV(b0), DescriptorTable(SRV(t8)), CBV(b2), DescriptorTable(SRV(t0)), DescriptorTable(SRV(t1)),"\
                        "DescriptorTable(SRV(t2)), DescriptorTable(SRV(t3)), DescriptorTable(SRV(t4)),"\
                        "DescriptorTable(SRV(t5), visibility=SHADER_VISIBILITY_PIXEL),"\
                        "DescriptorTable(SRV(t6), visibility=SHADER_VISIBILITY_PIXEL),"\
                        "DescriptorTable(SRV(t7), visibility=SHADER_VISIBILITY_PIXEL),"\
                        "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, addressU=TEXTURE_ADDRESS_CLAMP, addressV"\
                        "=TEXTURE_ADDRESS_CLAMP, MinLOD=0, MaxLOD=3.402823466e+38f, borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK ),"\
                        "StaticSampler(s1, filter = FILTER_ANISOTROPIC,"\
                        "addressU=TEXTURE_ADDRESS_CLAMP,"\
                        "addressV=TEXTURE_ADDRESS_CLAMP,"\
                        "MinLOD=0, MaxLOD=3.402823466e+38f,"\
                        "borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK)"

#define LightingRootSig2 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"\
"CBV(b0), DescriptorTable(SRV(t0)),"\
"DescriptorTable(SRV(t1), visibility=SHADER_VISIBILITY_PIXEL),"\
"DescriptorTable(SRV(t2), visibility=SHADER_VISIBILITY_PIXEL),"\
"DescriptorTable(SRV(t3), visibility=SHADER_VISIBILITY_PIXEL),"\
"DescriptorTable(SRV(t4), visibility=SHADER_VISIBILITY_PIXEL)"
#endif

#ifdef __cplusplus
#include <directxtk12/SimpleMath.h>
namespace ShaderData {
    using float4 = DirectX::SimpleMath::Vector4;
    using float3 = DirectX::SimpleMath::Vector3;
    using float2 = DirectX::SimpleMath::Vector2;
    using matrix = DirectX::SimpleMath::Matrix;
    struct Object
#else
cbuffer Object : register(b2)
#endif
{
    matrix ModelTr;
    matrix NormalTr;
    float3 diffuse;
    bool Textured;
    float3 specular;
    float roughness;
};

#ifdef __cplusplus
struct Constants
#else
cbuffer Constants : register(b0)
#endif
{
    matrix WorldView;
    matrix WorldInverse;
    matrix WorldProj;
    float3 CameraPos;
    int shaderMode;
    float4 hammersley[20];
    float momentBias;
    float depthBias;
};

#ifdef __cplusplus
struct Light
#else
struct Light
#endif
{
    matrix ShadowView;
    matrix ShadowProj;
    float3 lightPos;
    float ShadowMin;
    float3 lightColor;
    float ShadowMax;
    int useShadows;
    float range;
};

#define WIDTH 50
#ifdef AO
#define gwidth 32
#ifdef __cplusplus
struct AoData 
#else
cbuffer AoData : register(b0)
#endif
{
    float4 weights[(2 * WIDTH) / 4 + 1];
    float R;
    float n;
    float s;
    float k;
};
#endif

#ifdef COMPUTE
#ifdef __cplusplus
struct ComputeData
#else
cbuffer ComputeData : register(b0)
#endif
{
    float4 weights[(2 * WIDTH) / 4 + 1];
    int cwidth;
};
#endif

#ifdef __cplusplus
}
#endif