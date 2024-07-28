#include "ComputeShaderData.h"

RWTexture2D<float4> InputMap : register(u0);
RWTexture2D<float4> OutputMap : register(u1);

ConstantBuffer<Constants> consts : register(b0);

SamplerState StaticSampler : register(s0);
[RootSignature(RootSig)]
[numthreads(128, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
#ifndef V
    //uint width = 0;
    //uint height = 0;
    //InputMap.GetDimensions(width, height);
    //float2 uv = float2((float) DTid.x / (float)width, (float) DTid.y / (float)height);
    //float4 sum = InputMap.Sample(StaticSampler, uv);
    //for (int i = 0; i < consts.r; i++)
    //{
    //    uv.x += ((float)i * pow(consts.r, consts.it)) / width;
    //    sum += InputMap.Sample(StaticSampler, uv);
    //}
    //OutputMap[DTid.xy] = sum;                                           
    float2 gpos = float2(DTid.y, DTid.x);
    for (int i = 1; i < 1024; i++)
    {                                                    
        float2 gpos2 = gpos - float2(1, 0);
        InputMap[gpos + float2(i, 0)] = InputMap[gpos + float2(i, 0)] + InputMap[gpos2 + float2(i, 0)];                 
    }
    //OutputMap[DTid.xy] = sum;
#else
    //uint width = 0;
    //uint height = 0;
    //InputMap.GetDimensions(width, height);
    //float2 uv = float2((float) DTid.y / width, (float) DTid.x / height);
    //float4 sum = InputMap.Sample(StaticSampler, uv);
    //for (int i = 0; i < consts.r; i++)
    //{
    //    uv.y += ((float)i * pow(consts.r, consts.it)) / height;
    //    sum += InputMap.Sample(StaticSampler, uv);
    //}
    //OutputMap[DTid.yx] = sum;
    
    float2 gpos = float2(DTid.x, DTid.y);
    for (int i = 1; i < 2048; i++)
    {                                                    
        float2 gpos2 = gpos - float2(0, 1);
        //OutputMap[gpos + float2(0, i)] = InputMap[gpos] + InputMap[gpos2]; 
        InputMap[gpos + float2(0, i)] = InputMap[gpos + float2(0, i)] + InputMap[gpos2 + float2(0, i)];
    }
#endif
}