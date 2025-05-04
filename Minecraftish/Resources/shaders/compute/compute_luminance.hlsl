#define TILE_SIZE 16

// Based on https://github.com/BruOp/bae/blob/master/examples/01-tonemapping/cs_lum_hist.sc

#define EPSILON 0.00001
// Taken from RTR vol 4 pg. 278
#define RGB_TO_LUM float3(0.2125, 0.7154, 0.0721)

Texture2D<float4> colorRt : register(t0);

RWByteAddressBuffer HistogramBuffer : register(u0);

cbuffer Params : register(b0)
{
    float2 u_params : packoffset(c0);
    uint2 RenderTargetSize : packoffset(c0.z);
};

groupshared uint histogramShared[256];

uint colorToBin(float3 hdrColor, float minLogLum, float inverseLogLumRange) {
  float lum = max(dot(hdrColor, RGB_TO_LUM), 0.01);
  return lum < EPSILON ? 0 : uint(clamp((log2(lum) - minLogLum) * inverseLogLumRange, 0.0, 1.0) * 254.0 + 1.0);
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 GlobalID : SV_DispatchThreadID, uint GroupID : SV_GroupIndex)
{   
	histogramShared[GroupID] = 0;
    GroupMemoryBarrierWithGroupSync();
		
	uint2 ScreenPosition = GlobalID.xy * 8;
	
	if (ScreenPosition.x < RenderTargetSize.x && ScreenPosition.y < RenderTargetSize.y)
	{
		float3 hdrColor = min(colorRt.Load(int3(ScreenPosition.xy, 0)).rgb, 2.5.xxx);
        InterlockedAdd(histogramShared[colorToBin(hdrColor, u_params.x, u_params.y)], 1);
    }
	
	GroupMemoryBarrierWithGroupSync();
	uint org;
	HistogramBuffer.InterlockedAdd(GroupID * 4 + 4, histogramShared[GroupID], org);
}