#include "AudioSourceBase.h"

using namespace ENGINE_NAMESPACE;

AudioSourceBase::AudioSourceBase()
{
	p_Sound = {};
}

void AudioSourceBase::Play()
{
	p_ShouldBePlaying = true;
	if (!p_IsSoundInitialized) return;
	ma_sound_start(&p_Sound);
}

void AudioSourceBase::Stop(bool fully, bool fadeOut, uint32_t fadeOutMillis)
{
	p_ShouldBePlaying = false;
	p_ShouldReset = fully;

	if (!p_IsSoundInitialized) return;

	if (fadeOut)
	{
		ma_sound_stop_with_fade_in_milliseconds(&p_Sound, fadeOutMillis);
		return;
	}

	ma_sound_stop(&p_Sound);
}

void AudioSourceBase::SetVolume(float volume)
{
	if (!p_IsSoundInitialized) return;
	ma_sound_set_volume(&p_Sound, volume);
}

void AudioSourceBase::SetPan(float pan)
{
	if (!p_IsSoundInitialized) return;
	ma_sound_set_pan(&p_Sound, pan);
}

void AudioSourceBase::SetPitch(float pitch)
{
	if (!p_IsSoundInitialized) return;
	ma_sound_set_pitch(&p_Sound, pitch);
}

bool AudioSourceBase::IsPlaying()
{
	if (!p_IsSoundInitialized) return false;
	return ma_sound_is_playing(&p_Sound);
}

void AudioSourceBase::AttachToNode(ma_node* pNode, uint32_t index, uint32_t inputIndex)
{
	if (!p_IsSoundInitialized) return;
	ma_node_detach_all_output_buses(&p_Sound);
	ma_node_attach_output_bus(pNode, index, &p_Sound, inputIndex);
}
