#include "Timer.h"
#include <chrono>

long Timer::nanoTime()
{
	std::chrono::system_clock::time_point begin = std::chrono::system_clock::now();
	auto since_epoch = begin.time_since_epoch();
	return (long)(std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count());
}
