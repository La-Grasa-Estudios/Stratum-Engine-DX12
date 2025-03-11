#include "RawAudioBuffer.h"

using namespace ENGINE_NAMESPACE;

RawAudioBuffer::RawAudioBuffer(uint32_t sampleRate, uint32_t channels, ma_format format, uint32_t sizeInFrames)
{
    m_SampleRate = sampleRate;
    m_Channels = channels;
    m_Format = format;
    ma_pcm_rb_init(format, channels, sizeInFrames, NULL, NULL, &m_Rb);
    ma_pcm_rb_set_sample_rate(&m_Rb, m_SampleRate);
}

RawAudioBuffer::~RawAudioBuffer()
{
    Free();
}

void RawAudioBuffer::Free()
{
    if (m_Freed) return;
    m_Freed = true;
    ma_pcm_rb_uninit(&m_Rb);
}

void RawAudioBuffer::Clear()
{
    ma_pcm_rb_reset(&m_Rb);
}

void RawAudioBuffer::SetMaxBuffers(uint32_t nbBuffers)
{
    m_MaxBuffers = nbBuffers;
}

void RawAudioBuffer::Queue(void* data, uint32_t size)
{
    void* ptr;

    uint32_t frames = size / m_Channels;
    ma_pcm_rb_acquire_write(&m_Rb, &frames, &ptr);

    size_t typeSize = sizeof(short);
    if (m_Format == ma_format_f32)
    {
        typeSize = sizeof(float);
    }
    uint64_t frameSize = typeSize * m_Channels * frames;

    memcpy(ptr, data, frameSize);

    ma_pcm_rb_commit_write(&m_Rb, frames);
}

ma_pcm_rb* RawAudioBuffer::GetDataSource()
{
    return &m_Rb;
}

bool RawAudioBuffer::CanWrite(uint32_t amountInBytes)
{
    return ma_pcm_rb_available_write(&m_Rb) >= amountInBytes / m_Channels;
}
