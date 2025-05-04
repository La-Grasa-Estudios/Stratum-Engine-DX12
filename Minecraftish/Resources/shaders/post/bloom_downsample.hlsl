struct v2f
{
	float4 ClipPos : SV_Position;
	float2 TexCoord : TEXCOORD0;
};


#ifdef STAGE_VERTEX

static const float4 quad[3] =
{
	float4(-1.0, -3.0, 0.0, 2.0),
    float4(3.0, 1.0, 2.0, 0.0),
    float4(-1.0, 1.0, 0.0, 0.0), // position, texcoord
};

v2f main(in uint vertexID : SV_VertexID)
{
    
	v2f output;
	output.ClipPos = float4(quad[vertexID].xy, 0.0, 1.0);
	output.TexCoord = quad[vertexID].zw;
	return output;
}

#endif

#ifdef STAGE_PIXEL

Texture2D srcTexture : register(t0);
SamplerState bloomSampler : register(s0);

cbuffer BloomParams : register(b0)
{
	int2 ScreenSize;
};

float4 main(v2f input) : SV_Target
{
	float2 srcTexelSize = 1.0 / ScreenSize;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    float3 a = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - 2*x, input.TexCoord.y + 2*y)).rgb;
    float3 b = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x,       input.TexCoord.y + 2*y)).rgb;
    float3 c = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + 2*x, input.TexCoord.y + 2*y)).rgb;
    float3 d = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - 2*x, input.TexCoord.y)).rgb;
    float3 e = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x,       input.TexCoord.y)).rgb;
    float3 f = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + 2*x, input.TexCoord.y)).rgb;
    float3 g = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - 2*x, input.TexCoord.y - 2*y)).rgb;
    float3 h = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x,       input.TexCoord.y - 2*y)).rgb;
    float3 i = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + 2*x, input.TexCoord.y - 2*y)).rgb;
    float3 j = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - x, input.TexCoord.y + y)).rgb;
    float3 k = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + x, input.TexCoord.y + y)).rgb;
    float3 l = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - x, input.TexCoord.y - y)).rgb;
	float3 m = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + x, input.TexCoord.y - y)).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
	float3 sam = e * 0.125;
    sam += (a+c+g+i)*0.03125;
    sam += (b+d+f+h)*0.0625;
    sam += (j+k+l+m)*0.125;
	return float4(sam, 1.0);
	
}

#endif