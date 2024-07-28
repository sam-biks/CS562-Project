#include "ShaderData.h"

static const float pi = 3.14159;
static const float pi2 = 2 * pi;

struct PixelIn
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;            
};

Texture2D WorldPosition : register(t1);
Texture2D Normal : register(t2);
Texture2D Diffuse : register(t3);
Texture2D SpecularAlpha : register(t4);

//Texture2D<unorm float4> ShadowMap : register(t4);
//Texture2D IrradianceMap : register(t5);
//Texture2D SpecularMap : register(t6);

//Texture2D<float> AOMap : register(t7);

SamplerState PointSampler : register(s0);
SamplerState AnisoSampler : register(s1);

StructuredBuffer<Light> Lights : register(t0);

//float4 GetOptimizedDepth(float2 uv)
//{
//    float4 d = ShadowMap.SampleLevel(PointSampler, uv, 0);
//    d[0]-=0.035955884801f;
//	d=mul(d,float4x4(
//			0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
//			0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f,
//			0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
//			0.0319417555f,-0.1722823173f,-0.2758014811f,-0.3376131734f));
//    return d;
//}

float3 CholeskyDecomposition(float3 m1, float3 m2, float3 m3, float3 z)
{
    float m = 0.0001f;
    float a = max(sqrt(m1.x), m);
    float b = m1.y / a;
    float c = m1.z / a;
    float d = max(sqrt(abs(m2.y - (b * b))), m);
    float e = (m2.z - b * c) / d;
    float f = max(sqrt(abs(m3.z - (c * c) - (e * e))), m);
    
    float ch1 = z.x / a;
    float ch2 = (z.y - b * ch1) / d;
    float ch3 = (z.z - c * ch1 - e * ch2) / f;
    
    float c3 = ch3 / f;
    float c2 = (ch2 - e * c3) / d;
    float c1 = (ch1 - b * c2 - c * c3) / a;
    return float3(c1, c2, c3);
}

//static const float alpha = .00003f;
//float ComputeShadowCoefficient(float2 shadowIndex, float t)
//{
//    float4 depth = GetOptimizedDepth(shadowIndex);
//    float4 b = (1.f - momentBias) * depth + momentBias * 0.5f;
//    float3 c = CholeskyDecomposition(
//        float3(1, b.x, b.y),
//        float3(b.x, b.y, b.z),
//        float3(b.y, b.z, b.w),
//        float3(1, t, t * t)
//    );
//    float det = (c.y * c.y - 4 * c.z * c.x);
//    
//    det = sqrt(det);
//    float z2 = (-c.y - det) / (2 * c.z);
//    float z3 = (-c.y + det) / (2 * c.z);
//   
//    
//    if (t <= z2)
//    {
//        return 0;
//    }
//    
//    if (t <= z3)
//    {
//        float num = t * z3 - b.x * (t + z3) + b.y;
//        float den = (z3 - z2) * (t - z2);
//        return num / den;
//    }
//    
//    float num = z2 * z3 - b.x * (z2 + z3) + b.y;
//    float den = (t - z2) * (t - z3);
//    return 1.f - (num / den);
//}

float3 Fresnel(float3 Ks, float LH)
{
    return Ks + (1.f - Ks) * (pow(1.f - LH, 5));
}

float Distribution(float roughness, float NH)
{
    float num = roughness * roughness;                                       
    float tanTheta = sqrt(1.f - (NH * NH)) / NH;
    float denom = pi * pow(NH, 4) * pow(num + tanTheta * tanTheta, 2);
    return num / denom;
}

float GGXG1(float VN, float roughness)
{
    float tanTheta = sqrt(1.f - (VN * VN)) / max(VN, .0001f);
    return 2.f / (1.f + sqrt(1.f + (roughness * roughness) * (tanTheta * tanTheta)));
}

float Geometry(float LN, float VN, float roughness)
{
    return GGXG1(LN, roughness) * GGXG1(VN, roughness);
}

float2 UVOF(float3 w)
{
    float u = 0.5f + atan2(w.x, -w.z) / pi2;
    return float2(u, acos(w.y) / pi);
    //return float2(0.5f - atan2(w.y, w.x) / pi2, acos(w.z) / pi);
}

float3 VectorOf(float2 uv)
{
    float x = cos(pi2 * (0.5f - uv.x)) * sin(pi * uv.y);
    float y = sin(pi2 * (0.5f - uv.x)) * sin(pi * uv.y);
    float z = cos(pi * uv.y);
    return float3(x, y, z);
}

//float3 Irradiance(float3 N)
//{
//    return IrradianceMap.Sample(PointSampler, UVOF(N)).rgb;
//}

float Skew(float r2, float roughness)
{
    return atan((roughness * sqrt(r2)) / sqrt(1.f - r2));
    //return atan(sqrt(-(roughness * roughness) * log(1 - r2)));
}

float3 VectorFromRandom(float2 uv, float3 R)
{
    float3 D = VectorOf(uv);
    float3 y = abs(R.y) < 0.999f ? float3(0, 0, 1) : float3(0, 0, 1);
    float3 A = normalize(cross(float3(0, 0, 1), R));

    float3 B = normalize(cross(R, A));
    float3 W = normalize(D.x * A + D.y * B + D.z * R);
    return W;
}

float RadicalInverse(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
float2 Hammersley(uint i, uint N)
{
    return float2(float(i)/float(N), RadicalInverse(i));
}  

float3 Importance(float2 UV, float3 N, float roughness)
{
    float a = roughness;
    
    float phi = 2.f * pi * UV.y;
    float cosTheta = sqrt((1.f - UV.x) / (1.f + (a * a - 1.f) * UV.x));
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    
    float3 H = { cos(phi) * sinTheta, cosTheta, sin(phi) * sinTheta };

    float3 u = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 t = normalize(cross(N, u));
    float3 b = cross(N, t);
    
    return normalize(H.z * t + H.y * N + H.x * b);
}

//float3 SpecularIrradiance(
//    float roughness,
//    float3 R, float D, float3 Ks,
//    float3 V, float VN, float3 N
//)
//{
//    uint width, height;
//    //SpecularMap.GetDimensions(width, height);
//    float level = (0.5f * log2((width * height) / 40));
//    float3 sum = 0;
//    float w = 1;
//    for (int i = 0; i < 40; i++)
//    {
//        //float4 st = hammersley[i / 2];
//        //float2 r = { st[(i * 2) % 4], st[(i * 2 + 1) % 4] };
//        //
//        //float Theta = Skew(r.y, roughness);
//        //float3 W = VectorFromRandom(float2(0.05f, 0.0f), R);
//        //
//        ////float2 uv = UVOF(float3(-W.z, -W.x, W.y));
//        //float2 uv = UVOF(W);
//        //float3 H = normalize(W + V);
//        //float WH = dot(W, H);
//        //float WN = dot(W, N);
//        //float VH = dot(V, H);
//        //
//        //float G = Geometry(WH, VH, roughness);
//        //float D = Distribution(roughness, dot(N, H));
//        //float3 F = Fresnel(Ks, WH);
//        //
//        //float3 result = ((G * F) / (4 * WN * VN));
//        //result *= SpecularMap.SampleLevel(AnisoSampler, uv, 0).rgb;//level - (0.5f * log2(D) + 0)).rgb;
//        ////result *= cos(Theta);
//        //sum += W;
//        float2 uv = Hammersley(i, 40);
//        float3 H = Importance(uv, N, roughness);
//        float3 W = normalize(2 * dot(V, H) * H - V);
//        
//        float NW = max(dot(N, W), 0);
//        float WH = max(dot(W, H), 0);
//        float VH = max(dot(V, H), 0);
//        float NH = max(dot(N, H), 0);
//        if (NW > 0 && VN > 0 && VH > 0 && NH > 0 && WH > 0)
//        {
//            float2 UV = UVOF(W);
//            float G = Geometry(NW, VN, roughness);
//            float D = Distribution(roughness, NH);
//            float3 F = Fresnel(Ks, WH);
//            
//            float weight = VH / (VN * NH);
//            float3 result = F * G * weight;//((G * F) / (4 * NW * VN));
//            result *= SpecularMap.SampleLevel(AnisoSampler, UV, level - (0.5f * log2(D / 4) + 1)).rgb;
//            sum += result;
//            w += NW;
//        }
//    }            
//    sum /= 40;
//    return sum;
//}

float4 main(PixelIn _input) : SV_TARGET
{
    //if (shaderMode == 1)
    //{
    //    return float4(worldPosition, 1);
    //    //float d = WorldPosition.Sample(AnisoSampler, _input.texCoord).w;
    //    //return float4(d / 50, d / 50, d / 50, 1);
    //}
    //if (shaderMode == 2)
    //{
    //    return float4((normal), 1);
    //}
        
    //if (shaderMode == 3)
    //{
    //    return float4(diffuse, 1);
    //}
    float4 specularAlpha = SpecularAlpha.mips[0][_input.position.xy];//SpecularAlpha.Sample(PointSampler, _input.texCoord);
    //if (shaderMode == 4)
    //{
    //    return float4(specularAlpha.w, specularAlpha.w, specularAlpha.w, 1);
    //}
    //if (shaderMode == 5)
    //{
    //    float ao = AOMap.mips[0][_input.position.xy];//.Sample(PointSampler, _input.texCoord);
    //    return float4(ao, ao, ao, 1);
    //}
    float3 diffuse = Diffuse.mips[0][_input.position.xy].xyz;//Diffuse.Sample(PointSampler, _input.texCoord).xyz;
    const float e = 10;//2.71828f;
    if (specularAlpha.x == 0 && specularAlpha.y == 0 && specularAlpha.z == 0)
    {
        //if (Lights[_input.instance].useShadows)
        //{                             
        //    //float3 eC = e * diffuse;
        //    //diffuse = pow(eC / (eC + float3(1, 1, 1)), 1.f / 2.2f);
        //}
        //else
        //    return 0;
       return float4(diffuse, 1);
    }
    float3 worldPosition = WorldPosition.mips[0][_input.position.xy].xyz;//WorldPosition.Sample(PointSampler, _input.texCoord).xyz;
    float3 normal = Normal.mips[0][_input.position.xy].xyz;//Normal.Sample(PointSampler, _input.texCoord).xyz;
    //matrix ShadowProj = Lights[_input.instance].ShadowProj;
    //matrix ShadowView = Lights[_input.instance].ShadowView;
    float3 specular = specularAlpha.xyz;
    float roughness = specularAlpha.w;
    float3 N = normalize(normal);
    float3 V = normalize(CameraPos - worldPosition);
    float3 Kd = diffuse;
    float3 finalColor = 0;
    for (int i  = 0; i < 1025; i++)
    {
        float3 lightPos = Lights[i].lightPos;
        float3 lightColor = Lights[i].lightColor;
        float range = Lights[i].range;
        float dist = length(lightPos - worldPosition);
        if (dist > range)
            continue;
        float3 L = normalize(lightPos - worldPosition);
        float3 H = normalize(L + V);
        float NL = max(dot(N, L), 0);
        float HN = max(dot(H, N), 0);
        float att = (10.f / (dist * dist)) - (10.f / (range * range));
        finalColor += (Kd / pi) * lightColor * saturate(NL) * att;
        finalColor += lightColor * specular * pow(HN, roughness) * att;
    }
    //float ShadowMin = Lights[_input.instance].ShadowMin;
    //float ShadowMax = Lights[_input.instance].ShadowMax;
    //float NV = (dot(N, V));
    //float LH = dot(L, H);
    //float VH = dot(V, H);
    
    //float4 shadowCoords = mul(ShadowProj, mul(ShadowView, float4(worldPosition, 1)));
    //float2 shadowIndex;
    //shadowIndex.x = (1.f + (shadowCoords.x / shadowCoords.w)) * 0.5f;
    //shadowIndex.y = (1.f - (shadowCoords.y / shadowCoords.w)) * 0.5f;
    //
    //float shadowDepth = (shadowCoords.w - ShadowMin) / (ShadowMax - ShadowMin);
    //
    //float shadowCoefficient = 1;
    //
    //if (Lights[_input.instance].useShadows)
    //{
    //    shadowCoefficient = 1.f - (ComputeShadowCoefficient(shadowIndex, shadowDepth - depthBias));
    //}
  
    
    //float3 BRDFDiffuse = (Kd / pi);
    ////float3 IBLDiffuse = (Kd / pi) * Irradiance(N) * 5.f;
    ////float ao = AOMap.mips[0][_input.position.xy];
    //
    //finalColor += BRDFDiffuse;   
    //float D = Distribution(roughness, HN);
    //if (NL > 0.0001f && NV > 0.0001f && LH > 0.0001f && VH > 0.0001f && HN > 0.0001f)
    //{
    //    float3 F = Fresnel(specular, LH);
    //    float G = Geometry(LH, VH, roughness);
    //    float3 BRDFSpecular = (D * F * G) / (4.f * NL * NV);
    //    finalColor += BRDFSpecular;
    //}
    //finalColor *= saturate(NL) * lightColor * att;
   
    //if (Lights[_input.instance].useShadows)
    //{
    //    float3 R = 2 * NV * N - V;
    //    float3 IBLSpecular = SpecularIrradiance(roughness, R, D, specular, V, NV, N);
    //    finalColor += IBLSpecular * ao;
    //}
    //if (Lights[_input.instance].useShadows)
    //    finalColor += IBLDiffuse * ao;
    
    
    //float3 eC = e * finalColor;
    //finalColor = pow(eC / (eC + float3(1, 1, 1)), 1.f / 2.2f);

    return float4(finalColor, 1);
}