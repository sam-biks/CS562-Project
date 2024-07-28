#define COMPUTE
#include "ShaderData.h"
#define cRootSig "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), CBV(b0)"
RWTexture2D<unorm float4> Map1 : register(u0);
RWTexture2D<unorm float4> Map2 : register(u1);

groupshared unorm float4 v[128 + (2 * WIDTH + 1)]; //229

[RootSignature(cRootSig)]
[numthreads(128, 1, 1)]
void main(uint3 _groupID : SV_GroupID, uint3 _dti : SV_DispatchThreadID, uint3 _groupTID : SV_GroupThreadID)
{
#ifndef V
    {
        uint width, height;
        Map1.GetDimensions(width, height);
        float2 gpos = _dti.xy;
        float i = _groupTID.x;
        if (gpos.x - cwidth < width)
            v[i] = Map1[gpos + float2(-cwidth, 0)];
        else
            v[i] = Map1[float2(width - 1, gpos.y)];
        if (i <= 2 * cwidth)
        {
            if (gpos.x + 128 - cwidth < width)
                v[i + 128] = Map1[gpos + float2(128 - cwidth, 0)];
            else
                v[i + 128] = Map1[float2(width - 1, gpos.y)];
        }
        GroupMemoryBarrierWithGroupSync();
        unorm float4 sum = float4(0, 0, 0, 0);
        for (int j = 0; j <= (2 * cwidth); j++)
        {
            sum += weights[j / 4][j % 4] * v[i + j];
        }
        Map2[gpos] = saturate(sum);
    }
#else
    {                                   
        uint width, height;
        Map1.GetDimensions(width, height);
        float2 gpos = _dti.yx;
        float i = _groupTID.x;
        if (gpos.y - cwidth < height)
            v[i] = Map1[gpos + float2(0, -cwidth)];
        else
            v[i] = Map1[float2(gpos.x, height - 1)];
        if (i <= 2 * cwidth)
        {
            if (gpos.y + 128 - cwidth < height)
                v[i + 128] = Map1[gpos + float2(0, 128 - cwidth)];
            else
                v[i + 128] = Map1[float2(gpos.x, height - 1)];
        }
        GroupMemoryBarrierWithGroupSync();
        unorm float4 sum = float4(0, 0, 0, 0);
        for (int j = 0; j <= (2 * cwidth); j++)
        {
            sum += weights[j / 4][j % 4] * v[i + j];
        }
        Map2[gpos] = saturate(sum);
    }
#endif
}