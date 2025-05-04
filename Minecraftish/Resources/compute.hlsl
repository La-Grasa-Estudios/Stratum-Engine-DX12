#define LOCAL_SIZE 16

RWTexture2D<unorm float4> s_target : register(u0);

[numthreads(LOCAL_SIZE, LOCAL_SIZE, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID) {
	float3 id = float3(dispatchID.xyz % 16);
	float4 col = float4(id / 16.0, 1.0);
	
    s_target[int2(dispatchID.xy)] = col;
}