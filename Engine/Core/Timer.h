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

	long nanoTime();

	float m_Delta;
	long m_Start;

};
