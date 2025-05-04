#pragma once

#include "AudioSourceBase.h"
#include "Core/Ref.h"

#define MAX_SOURCES 4096

BEGIN_ENGINE

class AudioEngine
{
public:
	void Init();
	void Shutdown();

	void Update();

	void AddSource(Ref<AudioSourceBase> source);

	ma_engine* GetEngine();

	inline static AudioEngine* s_Instance = NULL;

private:
	ma_engine m_Engine;
	Ref<AudioSourceBase> m_Sources[MAX_SOURCES];
};

END_ENGINE