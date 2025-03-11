#pragma once

#include "av/StreamReader.hpp"
#include "av/Scale.hpp"
#include "av/Resample.hpp"

#include "Sound/AudioEngine.h"
#include "Sound/RawAudioBuffer.h"

#include <semaphore>

#include "znmsp.h"

BEGIN_ENGINE

struct VideoParams
{
	int width;
	int height;
};

class VideoDecode
{
public:

	/// <summary>
	/// 
	/// </summary>
	/// <param name="path">Path to the video file</param>
	/// <param name="pEngine">Pointer to the engine object, can be NULL if no audio is enabled</param>
	VideoDecode(std::string_view path, AudioEngine* pEngine);
	~VideoDecode();

	void Step();
	void StepAudio(float time);

	void PushFrame(av::Ptr<av::Frame> frame);
	av::Ptr<av::Frame> GetFrame();
	bool Finished();
	VideoParams GetSize();

	float GetFrametime();
	void SetLoop(bool loop);

private:

	bool m_Initialized;
	bool m_Stopped;
	bool m_Finished;
	float m_Framerate;

	ma_sound m_SoundStream;
	Ref<RawAudioBuffer> m_AudioBuffer;

	std::vector<av::Ptr<av::Frame>> m_Queue;
	std::vector<av::Ptr<av::Frame>> m_Pending;

	std::vector<av::Ptr<av::Frame>> m_AudioQueue;
	std::vector<av::Ptr<av::Frame>> m_AudioPending;

	av::Ptr<av::StreamReader> m_Reader;
	av::Ptr<av::Scale> m_Sws;
	av::Ptr<av::Resample> m_Swr;
	av::Frame m_Frame;

	std::binary_semaphore m_QueueStop = std::binary_semaphore(1);
	std::binary_semaphore m_PendingStop = std::binary_semaphore(1);

	std::binary_semaphore m_AudioQueueStop = std::binary_semaphore(1);
	std::binary_semaphore m_AudioPendingStop = std::binary_semaphore(1);

};

END_ENGINE