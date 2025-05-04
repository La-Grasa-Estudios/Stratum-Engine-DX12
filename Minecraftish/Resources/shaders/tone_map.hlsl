#permutationbase LUMINANCE

#permutationcond @ALL
#permutationadd BLOOM

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
#include "math.hlsl"

Texture2D sHdrImage : register(t0);
Texture2D sAvgLum : register(t1);
#ifdef BLOOM
Texture2D sBloomImage : register(t2);
Texture2D sDirtLensImage : register(t3);
#endif

SamplerState sBilinearSampler : register(s0);

float3 aces(float3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float Tonemap_Uchimura(float x, float P, float a, float m, float l, float c, float b) {
    // Uchimura 2017, "HDR theory and practice"
    // Math: https://www.desmos.com/calculator/gslcdxvipg
    // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = m * pow(x / m, c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float Tonemap_Uchimura(float x) {
    const float P = 1.0;  // max display brightness
    const float a = 1.0;  // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4;  // linear section length
    const float c = 1.33; // black
    const float b = 0.0;  // pedestal
    return Tonemap_Uchimura(x, P, a, m, l, c, b);
}

float4 main(v2f input) : SV_Target
{
    const float gamma = 2.2;
	const float exposure = 1.0;
	
    float3 color = sHdrImage.Sample(sBilinearSampler, input.TexCoord).rgb;
	
    float lum = sAvgLum.Sample(sBilinearSampler, input.TexCoord).r;

    float3 Yxy = convertRGB2Yxy(color);

    Yxy.x /= (9.6 * lum + 0.0001);

    color = convertYxy2RGB(Yxy);
	
	color = max(color, 0.0.xxx);

#ifdef BLOOM
	float3 rawBloom = sBloomImage.Sample(sBilinearSampler, input.TexCoord).rgb * 2;
	float3 dirtyLens = color + sDirtLensImage.Sample(sBilinearSampler, input.TexCoord).rgb * rawBloom;	
	color = lerp(color, (dirtyLens + rawBloom) / 2.0, 0.05);	
#endif

    color.r = Tonemap_Uchimura(color.r);
    color.g = Tonemap_Uchimura(color.g);
    color.b = Tonemap_Uchimura(color.b);

	color = pow(color, (1.0 / gamma).xxx);
	
    return float4(color, 1.0);
}

#endif
