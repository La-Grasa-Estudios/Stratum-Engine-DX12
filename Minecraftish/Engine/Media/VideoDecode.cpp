#include "VideoDecode.h"

#include "Core/Logger.h"
#include "Core/JobManager.h"

using namespace ENGINE_NAMESPACE;

#define MAX_QUEUE_SIZE 2
#define MAX_AUDIO_QUEUE_SIZE 16

VideoDecode::VideoDecode(std::string_view path, AudioEngine* pEngine)
{
	m_Stopped = false;
	m_Initialized = false;
	m_Finished = false;

	m_Reader = av::StreamReader::create(path.data(), true).value();

	bool AudioEnabled = pEngine;

	if (!m_Reader) {
		m_Reader = av::StreamReader::create(path.data(), false).value();
		AudioEnabled = false;
		if (!m_Reader)
		    return;
	}

	auto width = m_Reader->frameWidth();
	auto height = m_Reader->frameHeight();
	auto wantedframerate = m_Reader->framerate();
	wantedframerate = av_inv_q(wantedframerate);
	m_Framerate = wantedframerate.num / (float)wantedframerate.den;
	auto pixFmt = m_Reader->pixFmt();

	if (pixFmt == AV_PIX_FMT_NONE)
	{
		pixFmt = AV_PIX_FMT_YUV420P;
	}

	for (int i = 0; i < MAX_QUEUE_SIZE; i++)
	{
		auto frame = av::makePtr<av::Frame>();
		AVFrame* pFrame = frame->native();

		pFrame->width = width;
		pFrame->height = height;
		pFrame->format = AV_PIX_FMT_ARGB;
		pFrame->pts = 0;

		/* allocate the buffers for the frame data */
		auto ret = av_frame_get_buffer(pFrame, 0);

		ret = av_frame_make_writable(pFrame);

		m_Pending.push_back(frame);
	}

	m_Sws = av::Scale::create(width, height, pixFmt, width, height, AV_PIX_FMT_ARGB).value();

	if (AudioEnabled)
	{

		auto channels = m_Reader->channels();
		auto rate = m_Reader->sampleRate();
		auto aformat = m_Reader->sampleFormat();
		auto bitRate = 128 * 1024;

		m_AudioBuffer = CreateRef<RawAudioBuffer>(44100, 1, ma_format_f32, 200000);
		m_Swr = av::Resample::create(channels, aformat, rate, 1, AV_SAMPLE_FMT_FLT, 44100).value();

		for (int i = 0; i < MAX_AUDIO_QUEUE_SIZE; i++)
		{
			auto audioFrame = av::makePtr<av::Frame>();
			auto aframe = audioFrame->native();

			AVChannelLayout layout;
			av_channel_layout_default(&layout, 1);

			aframe->ch_layout = layout;
			aframe->sample_rate = 44100;
			aframe->format = AV_SAMPLE_FMT_FLT;
			aframe->pts = 0;
			aframe->pkt_dts = 0;

			av_frame_make_writable(aframe);

			m_AudioPending.push_back(audioFrame);
		}

		int16_t tempData[256]{};

		m_AudioBuffer->Queue(tempData, 256);

		ma_sound_init_from_data_source(pEngine->GetEngine(), m_AudioBuffer->GetDataSource(), MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_STREAM, NULL, &m_SoundStream);
		ma_sound_set_looping(&m_SoundStream, true);
		ma_sound_start(&m_SoundStream);
	}

	m_Initialized = true;

}

VideoDecode::~VideoDecode()
{
	if (m_AudioBuffer)
	{
		ma_sound_stop(&m_SoundStream);
		ma_sound_uninit(&m_SoundStream);
	}
}

void VideoDecode::Step()
{
	if (m_Finished || m_Pending.empty()) return;

	JobManager::Dispatch(1, 1, [this](JobDispatchArgs args) {
		m_PendingStop.lock();

		if (m_Pending.empty())
		{
			m_PendingStop.unlock();
			return;
		}

		auto frame = m_Pending.back();
		m_Pending.pop_back();

		m_PendingStop.unlock();

		int readTimes = 1;
		bool performRead = true;

		av::Frame& refFrame = m_Frames[args.jobIndex];

		while (readTimes > 0)
		{
			readTimes--;

			if (performRead && !m_Reader->readFrame(refFrame, args.jobIndex).value())
			{
				m_Finished = true;
				break;
			}

			performRead = true;

			if (m_AudioBuffer && refFrame.type() == AVMEDIA_TYPE_AUDIO)
			{
				m_AudioPendingStop.lock();

				if (m_AudioPending.empty())
				{
					auto audioFrame = av::makePtr<av::Frame>();
					auto aframe = audioFrame->native();

					AVChannelLayout layout;
					av_channel_layout_default(&layout, 1);

					aframe->ch_layout = layout;
					aframe->sample_rate = 44100;
					aframe->format = AV_SAMPLE_FMT_FLT;
					aframe->pts = 0;
					aframe->pkt_dts = 0;

					av_frame_make_writable(aframe);

					m_AudioPending.push_back(audioFrame);
				}

				auto audioFrame = m_AudioPending.back();
				m_AudioPending.pop_back();

				m_AudioPendingStop.unlock();

				m_Swr->convert(refFrame, *audioFrame);
				audioFrame->native()->time_base.num = refFrame.native()->time_base.num;
				audioFrame->native()->time_base.den = refFrame.native()->time_base.den;
				audioFrame->native()->pts = refFrame.native()->pts;
				audioFrame->native()->best_effort_timestamp = refFrame.native()->best_effort_timestamp;

				m_AudioQueueStop.lock();

				m_AudioQueue.push_back(audioFrame);

				m_AudioQueueStop.unlock();

				if (!m_Reader->readFrame(refFrame, args.jobIndex).value())
				{
					m_Finished = true;
					break;
				}
				else {
					if (refFrame.type() == AVMEDIA_TYPE_AUDIO) {
						readTimes++;
						performRead = false;
						continue;
					}
				}

			}

			if (refFrame.type() == AVMEDIA_TYPE_VIDEO)
			{
				m_Sws->scale(refFrame, *frame);

				m_QueueStop.lock();

				m_Queue.push_back(frame);

				m_QueueStop.unlock();

				/*/
				if (!m_Pending.empty())
				{
					m_PendingStop.lock();

					frame = m_Pending.back();
					m_Pending.pop_back();

					m_PendingStop.unlock();

					readTimes++;
				}
				*/
			}
		}
	});

}

void VideoDecode::StepAudio(float frame)
{

	m_AudioQueueStop.lock();

	//if (!m_AudioQueue.empty()) Z_INFO("Audio queue size {}", (int)m_AudioQueue.size());

	//std::vector<float> buffer;

	//int timestamp = frame * 1000.0f;

	//Z_INFO("Tms {}", timestamp);

	for (int i = 0; i < m_AudioQueue.size(); i++)
	{
		auto ptr = m_AudioQueue[i];

		AVFrame* packet = ptr->native();

		//packet->
		//Z_INFO("Ptms {}, {}/{}", packet->pts, packet->time_base.den, packet->time_base.num)

		if (!m_AudioBuffer->CanWrite(packet->nb_samples))
		{
			continue;
		}

		m_AudioQueue.erase(m_AudioQueue.begin() + i);
		i--;

		float* data = (float*)packet->data[0];

		/*
		for (int k = 0; k < packet->nb_samples; k++)
		{
			buffer.push_back(data[k]);
		}
		*/

		m_AudioBuffer->Queue(data, packet->nb_samples);

		m_AudioPendingStop.lock();
		m_AudioPending.push_back(ptr);
		m_AudioPendingStop.unlock();
	}

	//if (!buffer.empty()) m_AudioBuffer->Queue(buffer.data(), buffer.size());

	if (!ma_sound_is_playing(&m_SoundStream))
	{
		ma_sound_start(&m_SoundStream);
	}

	//m_AudioQueue.clear();

	m_AudioQueueStop.unlock();

}

void VideoDecode::PushFrame(av::Ptr<av::Frame> frame)
{
	m_PendingStop.lock();
	m_Pending.push_back(frame);
	m_PendingStop.unlock();
}

av::Ptr<av::Frame> VideoDecode::GetFrame()
{

	if (m_Queue.empty()) {
		if (m_Finished)
		{
			m_Stopped = true;
		}
		return NULL;
	}

	m_QueueStop.lock();

	auto ptr = m_Queue[0];
	m_Queue.erase(m_Queue.begin());

	m_QueueStop.unlock();

	return ptr;
}

bool VideoDecode::Finished()
{
	return m_Stopped && m_Finished;
}

VideoParams VideoDecode::GetSize()
{
	if (!m_Reader) return { 0 , 0 };
	return { m_Reader->frameWidth(), m_Reader->frameHeight() };
}

float VideoDecode::GetFrametime()
{
	return m_Framerate;
}

void VideoDecode::SetLoop(bool loop)
{
	m_Reader->setRepeat(loop);
}
