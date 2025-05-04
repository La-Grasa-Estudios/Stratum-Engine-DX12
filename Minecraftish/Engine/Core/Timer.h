#pragma once

#include "znmsp.h"

class Timer {

public:

	DLLEXPORT Timer() {
		m_Start = nanoTime();
	}

	DLLEXPORT float Get() {
		return (float)(nanoTime() - m_Start) / 1000.0F / 1000.0F / 1000.0F;
	}

	DLLEXPORT int GetMillis() {
		return (int)((float)(nanoTime() - m_Start) / 1000.0F / 1000.0F);
	}

private:

	uint64_t nanoTime();

	float m_Delta;
	uint64_t m_Start;

};
