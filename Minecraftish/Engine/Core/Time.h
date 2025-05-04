#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class Time {

	inline static long now = 0L;
	inline static long now_cpu = 0L;
	inline static long now_gpu = 0L;
	inline static long now_update = 0L;

public:

	DLLEXPORT inline static float DeltaTime = 0.0F;
	DLLEXPORT inline static float GlobalTime = 0.0F; // Aproximate Time Since Startup
	DLLEXPORT inline static float FixedDeltaTime = 1.0F / 50.0F;

	DLLEXPORT inline static float CPUTime = 0.0F;
	DLLEXPORT inline static float GPUTime = 0.0F;
	DLLEXPORT inline static float UpdateTime = 0.0F;

	DLLEXPORT static long nanoTime();

	DLLEXPORT static void BeginProfile();
	DLLEXPORT static void EndProfile();

	DLLEXPORT static void BeginRender();
	DLLEXPORT static void EndRender();

	DLLEXPORT static void ClearCPU();
	DLLEXPORT static void BeginCPU();
	DLLEXPORT static void EndCPU();

	DLLEXPORT static void ClearGPU();
	DLLEXPORT static void BeginGPU();
	DLLEXPORT static void EndGPU();

	DLLEXPORT static void ClearUpdate();
	DLLEXPORT static void BeginUpdate();
	DLLEXPORT static void EndUpdate();

};

END_ENGINE
