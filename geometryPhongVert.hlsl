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
    float3 normalVec : NORMAL;
    float4 worldPosition : WORLDPOS;
    float2 texCoord : TEXCOORD;
};

[RootSignature(RootSig)]
VertexOut main(VertexInput _input)
{
    float3 eye = mul(WorldInverse, float4(0, 0, 0, 1)).xyz;
    VertexOut output;
    output.worldPosition = mul(ModelTr, float4(_input.vertex, 1));
    output.position = mul(WorldProj, mul(WorldView, mul(ModelTr, float4(_input.vertex, 1))));
    output.worldPosition.w = output.position.w;
    float3 worldPos = mul(ModelTr, float4(_input.vertex, 1)).xyz;
    output.normalVec = normalize(mul(NormalTr, float4(_input.normal, 0)).xyz);
    output.texCoord = _input.texCoords;
    
	return output;
}