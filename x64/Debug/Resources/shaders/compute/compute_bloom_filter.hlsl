#include "math.hlsl"
#define LOCAL_SIZE 16

cbuffer Params : register(b0)
{
    uint2 SourceSize;
	uint2 TargetSize;
};

Texture2D s_source_mip : register(t0);
Texture2D s_lum : register(t1);
RWTexture2D<float3> s_target : register(u0);

[numthreads(LOCAL_SIZE, LOCAL_SIZE, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
	
    int2 texel = int2(dispatchID.xy * 2);
	
    float4 col1 = s_source_mip.Load(int3(texel, 0), int2(0, 0));
    float4 col2 = s_source_mip.Load(int3(texel, 0), int2(1, 0));
    float4 col3 = s_source_mip.Load(int3(texel, 0), int2(0, 1));
    float4 col4 = s_source_mip.Load(int3(texel, 0), int2(1, 1));
	
    float4 col = (col1 + col2 + col3 + col4) / 4.0;
	
    float lum = s_lum.Load(int3(0, 0, 0)).r;

    float3 Yxy = convertRGB2Yxy(col.rgb);

    Yxy.x /= (9.6 * lum + 0.0001);

    col.rgb = clamp(convertYxy2RGB(Yxy), 0.0, 20.0);
	
    s_target[int2(dispatchID.xy)] = col;

}