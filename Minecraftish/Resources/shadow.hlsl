#ifdef STAGE_VERTEX

cbuffer Constants : register(b0)
{
    float4x4 MVP;
};

float4 main(in float3 aPosition : TEXCOORD0) : SV_Position
{
	return mul(MVP, float4(aPosition, 1.0));
}

#endif

#ifdef STAGE_PIXEL

void main()
{
	
}

#endif