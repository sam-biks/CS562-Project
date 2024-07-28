#define RootSig "DescriptorTable(SRV(t0)), DescriptorTable(UAV(u0)), StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT )"
Texture2D<unorm float4> Depth : register(t0);
RWTexture2D<unorm float4> ShadowMap : register(u0);
SamplerState StaticSampler : register(s0);

float4 GetOptimizedMoments(in float depth)
{
    float square = depth * depth;
    float4 moments = float4(depth, square, square * depth, square * square);
    float4 optimized = mul(moments, float4x4(-2.07224649f,    13.7948857237f,  0.105877704f,   9.7924062118f,
                                              32.23703778f,  -59.4683975703f, -1.9077466311f, -33.7652110555f,
                                             -68.571074599f,  82.0359750338f,  9.3496555107f,  47.9456096605f,
                                              39.3703274134f,-35.364903257f,  -6.6543490743f, -23.9728048165f));
    optimized[0] += 0.035955884801f;
    return optimized;
}

[RootSignature(RootSig)]
[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float2 uv = float2((float)DTid.x / 1024.f, (float)DTid.y / 1024.f);
    float4 depth = Depth.Sample(StaticSampler, uv);
   // float4 newDepth = float4(depth, depth * depth, depth * depth * depth, depth * depth * depth * depth);
    ShadowMap[DTid.xy] = GetOptimizedMoments(depth.r);
}