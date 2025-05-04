struct v2f
{
    float4 ClipPos : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float3 ShadowCoord : TEXCOORD1;
};

#ifdef STAGE_VERTEX

struct vs_in
{
    float3 aPosition : TEXCOORD0;
    float2 aTexCoord : TEXCOORD1;
};

cbuffer Constants : register(b0)
{
    float4x4 ModelMatrix;
};

cbuffer GlobalMatrices : register(b1)
{
	float4x4 ProjMatrix;
	float4x4 ViewMatrix;
	float4x4 ShadowPV;
};

v2f main(in vs_in input)
{
	float4x4 MVP = mul(ProjMatrix, mul(ViewMatrix, ModelMatrix));
    v2f output;
    output.ClipPos = mul(MVP, float4(input.aPosition, 1.0));
    output.TexCoord = input.aTexCoord;
    output.ShadowCoord = mul(mul(ShadowPV, ModelMatrix), float4(input.aPosition, 1.0));
    return output;
}

#endif

#ifdef STAGE_PIXEL

SamplerState samplerBilinear : register(s0);

Texture2D sAlbedo : register(t0);
Texture2D sShadowMap : register(t1);

float get_visibility(float dBlocker, float dReciever)
{
	const float Exponential = 64.0;
    float lightDepth = exp(dBlocker * Exponential);
    return pow(clamp(abs(lightDepth * exp(-dReciever * Exponential)), 0.0, 1.0), 64);
}

float InterleavedGradientNoise(float2 position_screen)
{
	float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
	return frac(magic.z * frac(dot(position_screen * 0.5 + 0.5, magic.xy)));
}

float2 VogelDiskOffset(int sampleIndex, int samplesCount, float phi)
{
  float GoldenAngle = 2.4;

  float r = sqrt(sampleIndex + 0.5) / sqrt(samplesCount);
  float theta = sampleIndex * GoldenAngle + phi * 521342.0;

  float sine, cosine;
  sine = sin(theta);
  cosine = cos(theta);
  
  return float2(r * cosine, r * sine) * 1;
}

float4 main(v2f input) : SV_Target
{
    float4 color = sAlbedo.Sample(samplerBilinear, input.TexCoord.xy);
	
	float2 shadowCoord = input.ShadowCoord.xy * 0.5 + 0.5;
	shadowCoord.y = 1 - shadowCoord.y;
	
	float noise = InterleavedGradientNoise(input.ClipPos.xy);
    float2 shadowTexelSize = (1.0 / 4096.0).xx;
	
	float shadow = 0.0;
	
	for (int i = 0; i < 32; i++)
	{
		float2 offset = VogelDiskOffset(i, 32, noise) * shadowTexelSize * 5.0;
		float depth = sShadowMap.Sample(samplerBilinear, shadowCoord + offset).r;
		shadow += get_visibility(depth, input.ShadowCoord.z) / 32.0;
	}
	
	color *= max(shadow, 0.1);
	
    return color;
}

#endif