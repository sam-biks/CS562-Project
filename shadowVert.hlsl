#include "ShaderData.h"

struct VertexInput
{
    float3 vertex : SV_Position;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;     
};

struct VertexOut
{
    float4 position : SV_Position;
    float4 pos : POSITION;
};

ConstantBuffer<Light> Lights : register(b1);

[RootSignature(RootSig)]
VertexOut main(VertexInput _input)
{
    VertexOut output;
    output.position = mul(Lights.ShadowProj, mul(Lights.ShadowView, mul(ModelTr, float4(_input.vertex, 1))));
    output.pos = output.position;
	return output;
}

float4 PSmain(VertexOut _in) : SV_Target
{
    float4 pos = _in.pos;
    float d = (pos.w - Lights.ShadowMin) / (Lights.ShadowMax - Lights.ShadowMin);
   
    return float4(d, d * d, d * d * d, d * d * d * d);
}