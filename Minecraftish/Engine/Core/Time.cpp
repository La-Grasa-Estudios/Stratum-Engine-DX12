#include "Time.h"

#include <chrono>

using namespace ENGINE_NAMESPACE;

long Time::nanoTime()
{
	std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
	auto since_epoch = begin.time_since_epoch();
	return (long)(std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count());
}

void Time::BeginProfile()
{
	now = nanoTime();
}

void Time::EndProfile()
{
	DeltaTime = (float)(nanoTime() - now) / 1000.0F / 1000.0F / 1000.0F;
	GlobalTime += DeltaTime;
}

void Time::BeginRender()
{
}

void Time::EndRender()
{
}

void Time::ClearCPU()
{
	CPUTime = 0.0F;
}

void Time::BeginCPU()
{
	now_cpu = nanoTime();
}

void Time::EndCPU()
{
	CPUTime = (float)(nanoTime() - now_cpu) / 1000.0F / 1000.0F / 1000.0F;
}

void Time::ClearGPU()
{
	GPUTime = 0.0F;
}

void Time::BeginGPU()
{
	now_gpu = nanoTime();
}

void Time::EndGPU()
{
	GPUTime = (float)(nanoTime() - now_gpu) / 1000.0F / 1000.0F / 1000.0F;
}

void Time::ClearUpdate()
{
	UpdateTime = 0.0f;
}

void Time::BeginUpdate()
{
	now_update = nanoTime();
}

void Time::EndUpdate()
{
	UpdateTime += (float)(nanoTime() - now_update) / 1000.0F / 1000.0F / 1000.0F;
}
