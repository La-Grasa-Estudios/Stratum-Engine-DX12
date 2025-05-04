#pragma once

#include "znmsp.h"

#include "miniaudio/miniaudio.h"
#include <vector>

BEGIN_ENGINE

class RawAudioBuffer
{
public:
	RawAudioBuffer(uint32_t sampleRate, uint32_t channels, ma_format format, uint32_t sizeInFrames);
	~RawAudioBuffer();

	void Free();
	void Clear();

	void SetMaxBuffers(uint32_t nbBuffers);

	/// <summary>
	/// Queues new data for the buffer
	/// </summary>
	/// <param name="data">The pointer to the data</param>
	/// <param name="size">The size of the data in samples (sizeInBytes / bytesPerSample)</param>
	void Queue(void* data, uint32_t size);
	ma_pcm_rb* GetDataSource();
	bool CanWrite(uint32_t amountInFrames);

private:

	ma_pcm_rb m_Rb;
	bool m_Freed = false;
	uint32_t m_SampleRate;
	uint32_t m_Channels;
	uint32_t m_MaxBuffers = 0;
	ma_format m_Format;

};

END_ENGINE