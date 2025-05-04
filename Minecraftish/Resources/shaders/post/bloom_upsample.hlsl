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
    float4(-1.0, 1.0, 0.0, 0.0), // position, input.TexCoord
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
static const float filterRadius = 0.003;

float4 main(v2f input) : SV_Target
{
    float x = filterRadius;
    float y = filterRadius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    float3 a = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - x, input.TexCoord.y + y)).rgb;
    float3 b = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x,     input.TexCoord.y + y)).rgb;
    float3 c = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + x, input.TexCoord.y + y)).rgb;
    float3 d = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - x, input.TexCoord.y)).rgb;
    float3 e = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x,     input.TexCoord.y)).rgb;
    float3 f = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + x, input.TexCoord.y)).rgb;
    float3 g = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x - x, input.TexCoord.y - y)).rgb;
    float3 h = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x,     input.TexCoord.y - y)).rgb;
    float3 i = srcTexture.Sample(bloomSampler, float2(input.TexCoord.x + x, input.TexCoord.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    float3 sam = e*4.0;
    sam += (b+d+f+h)*2.0;
    sam += (a+c+g+i);
    sam *= 1.0 / 16.0;
	return float4(sam, 1.0);
}

#endif