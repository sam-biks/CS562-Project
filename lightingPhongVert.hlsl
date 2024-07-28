#include "ShaderData.h"

struct VertexInput
{
    float3 vertex : SV_Position;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
    uint instance : SV_InstanceID;
};

struct VertexOut
{
    float4 position : SV_Position; 
    float2 texCoord : TEXCOORD;     
};

//[RootSignature(LightingRootSig)]
[RootSignature(LightingRootSig2)]
VertexOut main(VertexInput _input)
{
    VertexOut output;
    output.position = float4(_input.vertex, 1);
    output.texCoord = _input.texCoords;
	return output;
}