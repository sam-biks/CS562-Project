#define CRootSig "CBV(b0), DescriptorTable(SRV(t0)), DescriptorTable(SRV(t1)),"\
                "DescriptorTable(SRV(t2)), DescriptorTable(UAV(u0)),"\
                "StaticSampler(s0)"

#define AORootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), "\
                "CBV(b0), DescriptorTable(SRV(t0)),"\
                "DescriptorTable(SRV(t1)),"\
                "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_POINT, MinLOD=0, MaxLOD=0)"
#define AO
#include "ShaderData.h"                   
Texture2D<float4> WorldPosition : register(t0);
Texture2D<float4> Normal : register(t1);
Texture2D<float> AOMap : register(t2);
RWTexture2D<float> AOMap2 : register(u0);


SamplerState StaticSampler : register(s0);
static const float pi = 3.14159;

float H(float f)
{
    if (f < 0)
        return 0;
    else
        return 1;
}


//[RootSignature(CRootSig)]
//[numthreads(128, 1, 1)]
//void main(uint3 _groupID : SV_GroupID, uint3 _dti : SV_DispatchThreadID, uint3 _groupTID : SV_GroupThreadID)
//{
//    uint width, height;
//    WorldPosition.GetDimensions(width, height);
//    int2 ipos = _dti.xy;
//    float2 uv = float2(float(ipos.x) / float(1920.f), float(ipos.y) / float(1080));
//    
//    float3 P = WorldPosition.SampleLevel(StaticSampler, uv, 0).xyz;
//    float3 N = normalize(Normal.SampleLevel(StaticSampler, uv, 0).xyz);
//    float d = WorldPosition.SampleLevel(StaticSampler, uv, 0).w;
//    
//    const float c = 0.1f * R;
//    
//    float sum = 0;
//    float phi = (30 * ipos.x ^ ipos.y) + 10 * ipos.x * ipos.y;
//    for (int i = 0; i < n; i++)
//    {
//        float alpha = (i + .5f) / n;
//        float h = alpha * R / d;
//        float theta = 2.f * pi * alpha * (7.f * n / 9.f) + phi;
//        float2 uv2 = uv + h * float2(cos(theta), sin(theta));
//        float3 Pi = WorldPosition.SampleLevel(StaticSampler, uv2, 0).xyz;
//        float di = WorldPosition.SampleLevel(StaticSampler, uv2, 0).w;
//        
//        float3 Wi = (Pi - P);
//        float sigma = 0.001f;
//        
//        float num = max(0, dot(N, normalize(Wi)) - sigma * di) * H(R - length(Wi));
//        float den = max(c * c, dot(Wi, Wi));
//        sum += num / den;
//    }
//    
//    float S = ((pi * 2 * c) / n) * sum;
//    AOMap[ipos] = pow(max(0, 1.f - s * S), k);
//}

struct VertexInput
{
    float3 vertex : SV_Position;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;     
};

struct VertexOut
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};
[RootSignature(AORootSig)]
VertexOut VSMain(VertexInput _input)
{
    VertexOut vout;
    vout.position = float4(_input.vertex, 1);
    vout.texCoord = _input.texCoords;
    return vout;
}

float PSMain(VertexOut _input) : SV_Target
{
    uint width, height;
    WorldPosition.GetDimensions(width, height);
    float4 Pd = WorldPosition.SampleLevel(StaticSampler, _input.texCoord, 0);
    float3 N = normalize(Normal.SampleLevel(StaticSampler, _input.texCoord, 0).xyz);
    float3 P = Pd.xyz;
    float d = Pd.w;                                                            
    
    const float c = 0.1f * R;
    
    float sum = 0;
    float phi = (30 * int(_input.position.x) ^ int(_input.position.y)) + 10 * int(_input.position.x) * int(_input.position.y);
    for (int i = 0; i < n; i++)
    {
        float alpha = (i + .5f) / n;
        float h = alpha * R / d;
        float theta = 2.f * pi * alpha * (7.f * n / 9.f) + phi;
        float2 uv2 = _input.texCoord + h * float2(cos(theta), sin(theta));
        float4 Pid = WorldPosition.SampleLevel(StaticSampler, uv2, 0); 
        float3 Pi = Pid.xyz;
        float di = Pid.w;
        
        float3 Wi = (Pi - P);
        float sigma = 0.001f;
        
        float num = max(0, dot(N, normalize(Wi)) - sigma * di) * H(R - length(Wi));
        float den = max(c * c, dot(Wi, Wi));
        sum += num / den;
    }
    
    float S = ((pi * 2 * c) / n) * sum;
    return pow(max(0, 1.f - s * S), k);
}

groupshared float v[128 + (2 * WIDTH + 1)]; //229
groupshared float4 Normals[128 + (2 * WIDTH + 1)];

float Rf(float4 NI, float4 NIi, float s)
{
    const float e = 2.718281828f;
    float3 N = NI.rgb;
    float3 Ni = NIi.rgb;
    float d = NI.w;
    float di = NIi.w;
    
    float r = max(0, dot(N, Ni));
    r /= sqrt(2 * pi * s);
    r *= pow(e, -((di - d) * (di - d)) / (2 * s));
    
    return r;
}
[RootSignature(CRootSig)]
[numthreads(128, 1, 1)]
void AOBlurmain(uint3 _groupID : SV_GroupID, uint3 _dti : SV_DispatchThreadID, uint3 _groupTID : SV_GroupThreadID)
{
    uint width, height;
    AOMap.GetDimensions(width, height);
    float2 gpos = _dti.xy;
    float i = _groupTID.x;
    if (gpos.x - gwidth < width)
    {
        v[i] = AOMap.mips[0][gpos + float2(-gwidth, 0)];
        Normals[i] = Normal.mips[0][gpos + float2(-gwidth, 0)];
    }
    else
    {
        v[i] = AOMap.mips[0][float2(width - 1, gpos.y)];
        Normals[i] = Normal.mips[0][float2(width - 1, gpos.y)];
    }
    if (i <= 2 * gwidth)
    {
        if (gpos.x + 128 - gwidth < width)
        {
            v[i + 128] = AOMap.mips[0][gpos + float2(128 - gwidth, 0)];
            Normals[i + 128] = Normal.mips[0][gpos + float2(128 - gwidth, 0)];
        }
        else
        {
            v[i + 128] = AOMap.mips[0][float2(width - 1, gpos.y)];
             Normals[i + 128] = Normal.mips[0][float2(width - 1, gpos.y)];
        }
    }
    GroupMemoryBarrierWithGroupSync();
    float sum = 0;
    float den = 0;
    float2 uv = float2(float(gpos.x) / float(width), float(gpos.y) / float(height));
    const float s = .01f;
    float4 N = Normals[i + gwidth];
    for (int j = 0; j <= (2 * gwidth); j++)
    {
        float2 uv2 = uv + float2(float(-gwidth + j) / float(width), 0);
        sum += weights[j / 4][j % 4] * v[i + j] * Rf(N, Normals[i + j], s);
        den += weights[j / 4][j % 4] * Rf(N, Normals[i + j], s);
    }
    AOMap2[gpos] = sum / den;// / den; //saturate(sum);
    //uint width, height;
    //AOMap.GetDimensions(width, height);
    //float2 gpos = _dti.xy;
    //
    //float2 uv = float2(float(gpos.x - gwidth) / float(width), float(gpos.y));
    //const float s = .01f;
    //float sum =0;
    //float den = 0;
    //for (int j = 0; j <= (2 * gwidth); j++)
    //{
    //    float2 uv2 = uv + float2(float(j) / float(width), 0);
    //    sum += weights[j / 4][j % 4] * AOMap[gpos + float2(-gwidth + j, 0)] * Rf(uv, uv2, s);
    //    den += weights[j / 4][j % 4] * Rf(uv, uv2, s);
    //}
    //AOMap2[gpos] = sum / den;
}

[RootSignature(CRootSig)]
[numthreads(1, 128, 1)]
void AOBlurmainV(uint3 _groupID : SV_GroupID, uint3 _dti : SV_DispatchThreadID, uint3 _groupTID : SV_GroupThreadID)
{
    uint width, height;
    AOMap.GetDimensions(width, height);
    float2 gpos = _dti.xy;
    float i = _groupTID.y;
    if (gpos.x - gwidth < width)
    {
        v[i] = AOMap.mips[0][gpos + float2(0, -gwidth)];
        Normals[i] = Normal.mips[0][gpos + float2(0, -gwidth)];
    }
    else
    {
        v[i] = AOMap.mips[0][float2(gpos.x, height - 1)];
        Normals[i] = Normal.mips[0][float2(gpos.x, height - 1)];
    }
    if (i <= 2 * gwidth)
    {
        if (gpos.y + 128 - gwidth < height)
        {
            v[i + 128] = AOMap.mips[0][gpos + float2(0, 128 - gwidth)];
            Normals[i + 128] = Normal.mips[0][gpos + float2(0, 128 - gwidth)];
        }
        else
        {
            v[i + 128] = AOMap.mips[0][float2(gpos.x, height - 1)];
             Normals[i + 128] = Normal.mips[0][float2(gpos.x, height - 1)];
        }
    }
    GroupMemoryBarrierWithGroupSync();
    float sum = 0;
    float den = 0;
    float2 uv = float2(float(gpos.x) / float(width), float(gpos.y) / float(height));
    const float s = .01f;
    float4 N = Normals[i + gwidth];
    for (int j = 0; j <= (2 * gwidth); j++)
    {
        float2 uv2 = uv + float2(0, float(-gwidth + j) / float(height));
        sum += weights[j / 4][j % 4] * v[i + j] * Rf(N, Normals[i + j], s);
        den += weights[j / 4][j % 4] * Rf(N, Normals[i + j], s);
    }
    AOMap2[gpos] = sum / den;

}