#pragma once

#include "miniaudio/miniaudio.h"
#include "znmsp.h"

BEGIN_ENGINE

class AudioSourceBase
{

public:

	AudioSourceBase();

	void Play();
	void Stop(bool fully = false, bool fadeOut = false, uint32_t fadeOutMillis = 100);
	virtual void Rewind() {}

	void SetVolume(float volume);
	float GetVolume();

	void SetPan(float pan);
	float GetPan();

	void SetPitch(float pitch);
	float GetPitch();

	bool IsPlaying();

	virtual void UpdateSource() {}
	virtual void AttachToNode(ma_node* pNode, uint32_t index, uint32_t inputIndex);
	
	bool Removed = false;

protected:

	ma_sound p_Sound;

	bool p_ShouldBePlaying = false;
	bool p_ShouldReset = false;
	bool p_IsSoundInitialized = false;

};

END_ENGINE