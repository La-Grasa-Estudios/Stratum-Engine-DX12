struct v2f
{
    float4 clipPosition : SV_Position;
    float2 texCoord : TEXCOORD0;
};

#ifdef STAGE_VERTEX

const float4 quad[3] = {
    float4(-1.0, -3.0, 0.0, 2.0),
    float4(3.0, 1.0, 2.0, 0.0),
    float4(-1.0, 1.0, 0.0, 0.0), // position, texcoord
};

v2f main(in uint vertexID : SV_VertexID) {
    
    v2f output;
    output.clipPosition = float4(quad[vertexID].xy, 0.0, 1.0);
    output.texCoord = quad[vertexID].zw;
    return output;
}

#endif

#ifdef STAGE_PIXEL

Texture2D SrcSurface : TEXTURE : register(t0);
SamplerState TextureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

float4 main(v2f input) : SV_Target
{
    return SrcSurface.Sample(TextureSampler, input.texCoord);
}

#endif