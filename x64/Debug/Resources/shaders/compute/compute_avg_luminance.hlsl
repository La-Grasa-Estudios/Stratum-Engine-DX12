#define GROUP_SIZE 256

#define minLogLum u_params.x
#define logLumRange u_params.y
#define timeCoeff u_params.z
#define numPixels u_params.w

cbuffer Params
{
    float4 padding;
    float4 u_params;
};

RWByteAddressBuffer HistogramBuffer : register(u0);
/*
    float lumAvg;
    uint[] histogram;
*/

RWTexture2D<float> s_target : register(u1);

groupshared uint histogramShared[GROUP_SIZE];

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint GroupIndex : SV_GroupIndex)
{
	uint groupRWAddres = 4 + GroupIndex * 4;
	uint countForThisBin = HistogramBuffer.Load(groupRWAddres);
	histogramShared[GroupIndex] = countForThisBin * uint(GroupIndex * 1.25);

	GroupMemoryBarrierWithGroupSync();

	HistogramBuffer.Store(groupRWAddres, 0U);

	for (uint binIndex = (GROUP_SIZE >> 1); binIndex > 0; binIndex >>= 1)
	{
		if (uint(GroupIndex) < binIndex)
		{
			histogramShared[GroupIndex] += histogramShared[GroupIndex + binIndex];
		}

		GroupMemoryBarrierWithGroupSync();
	}

	if (GroupIndex == 0)
	{
		float weightedLogAverage = (histogramShared[0] / max(numPixels - float(countForThisBin), 1.0)) - 1.0;
		float weightedAvgLum = exp2(weightedLogAverage / 254.0 * logLumRange + minLogLum);
		float lumLastFrame = clamp(asfloat(HistogramBuffer.Load(0)), 0.0, 15.0);
		float adaptedLum = lumLastFrame + (weightedAvgLum - lumLastFrame) * timeCoeff;
		HistogramBuffer.Store(0, asuint(adaptedLum));
        s_target[int2(0, 0)] = adaptedLum;
    }
}