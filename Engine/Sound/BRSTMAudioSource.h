#pragma once

#include "RawAudioBuffer.h"
#include "AudioSourceBase.h"
#include "brstm_codec.h"

BEGIN_ENGINE

class BRSTMAudioSource : public AudioSourceBase
{

public:

	BRSTMAudioSource(BRSTM::BRSTMFile* pFile, ma_engine* pEngine);
	~BRSTMAudioSource();

	void Rewind() override;
	void UpdateSource() override;

private:

	bool TryProcessChunk();

	BRSTM::BRSTMStream* m_Stream;
	BRSTM::BRSTMChunk* m_NextChunk;
	uint32_t m_LoopChunk = 0;
	uint32_t m_LoopSamples = 0;
	uint32_t m_LoopCount = 0;
	ma_engine* m_Engine;
	RawAudioBuffer* m_AudioBuffer;

};

END_ENGINE