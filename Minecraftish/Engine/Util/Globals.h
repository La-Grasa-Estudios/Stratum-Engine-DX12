#pragma once

#include "znmsp.h"

BEGIN_ENGINE

struct GlobalVars
{
	uint64_t gametic;
	uint64_t tickRate;
	float deltaTime;
};

extern GlobalVars* gpGlobals;

END_ENGINE