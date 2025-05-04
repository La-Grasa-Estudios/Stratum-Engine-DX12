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

Texture2D<float4> videoSurface : register(t0);
SamplerState videoSampler : register(s0);

float4 main(v2f input) : SV_Target {
	return videoSurface.Sample(videoSampler, input.TexCoord).gbar;
}

#endif