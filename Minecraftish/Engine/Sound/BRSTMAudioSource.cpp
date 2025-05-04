#include "BRSTMAudioSource.h"

using namespace ENGINE_NAMESPACE;

BRSTMAudioSource::BRSTMAudioSource(BRSTM::BRSTMFile* pFile, ma_engine* pEngine)
{
	m_Stream = new BRSTM::BRSTMStream(pFile);
	m_Engine = pEngine;
    m_NextChunk = NULL;
    m_AudioBuffer = NULL;
}

BRSTMAudioSource::~BRSTMAudioSource()
{
	delete m_Stream;
	delete m_AudioBuffer;
    if (p_IsSoundInitialized) ma_sound_uninit(&p_Sound);
}

void BRSTMAudioSource::Rewind()
{
	Stop();
    if (m_AudioBuffer) m_AudioBuffer->Clear();
	m_Stream->rewind();
    if (m_NextChunk) delete m_NextChunk;
    m_NextChunk = NULL;
    m_LoopCount = 0;
	//Play();
}

void BRSTMAudioSource::UpdateSource()
{
    while (TryProcessChunk() || !m_NextChunk)
    {
        ENGINE_NAMESPACE::BRSTM::BRSTMChunk* chunk = NULL;

        if (!(chunk = m_Stream->get_next_chunk())) {
            if (m_Stream->file->header.loop) {
                m_Stream->rewind();
                chunk = m_Stream->get_next_chunk();
            }
            else
            {
                return;
            }
        }

        m_NextChunk = chunk;
    }
}

bool BRSTMAudioSource::TryProcessChunk()
{

    if (p_ShouldReset)
    {
        p_ShouldReset = false;
        Rewind();
    }

    if (!m_NextChunk || !p_ShouldBePlaying) return false;

    int samples = m_Stream->positionInSamples;

    uint32_t waveSize = m_NextChunk->waveHeader.subchunk2Size;
    uint32_t waveOffset = 0;

    if (m_Stream->file->header.loop) {

        uint32_t loopStart = m_Stream->file->header.loopStart;
        uint32_t loopEnd = m_Stream->file->header.loopSize;
        uint32_t loopChunk = loopEnd - (loopEnd % m_NextChunk->waveHeader.sampleRate);

        if (samples + m_NextChunk->get_chunk_samples() >= loopStart && m_LoopChunk == 0) {

            m_LoopChunk = m_Stream->filePosition - m_NextChunk->get_chunk_size();
            m_LoopSamples = samples;
           // std::cout << "Loop pos " << m_LoopChunk << " Samples " << m_LoopSamples << "\n";

        }

        if (samples == m_LoopSamples && m_LoopSamples != 0 && m_LoopCount != 0) {

            int32_t extraSamples = loopStart - m_LoopSamples;

            if (extraSamples > 0) {

                int32_t extraBytes = extraSamples * (m_NextChunk->waveHeader.bitsPerSample / 8) * m_NextChunk->waveHeader.numChannels;
                waveOffset = extraBytes;
                waveSize -= extraBytes;

            }

            //std::cout << "Loop start " << loopStart << " loop at file " << m_LoopSamples << " Samples " << extraSamples << "\n";

        }

        if ((uint32_t)samples >= loopChunk) {

            int32_t extraSamples = m_NextChunk->get_chunk_samples() - (loopEnd % m_NextChunk->waveHeader.sampleRate);
            if (extraSamples > 0) {
                int32_t extraBytes = extraSamples * (m_NextChunk->waveHeader.bitsPerSample / 8) * m_NextChunk->waveHeader.numChannels;
                waveSize -= extraBytes;

            }

            waveOffset = 0;

            m_Stream->rewind();

            m_Stream->filePosition = m_LoopChunk;
            m_Stream->positionInSamples = m_LoopSamples;

            //std::cout << "Looping at " << samples << " extra data is " << extraSamples << " chunk size is " << m_NextChunk->get_chunk_samples() << "\n";

            m_LoopCount++;

        }

    }

    if (!m_AudioBuffer)
    {
        m_AudioBuffer = new Stratum::RawAudioBuffer(m_NextChunk->waveHeader.sampleRate, m_NextChunk->waveHeader.numChannels, m_NextChunk->waveHeader.bitsPerSample == 16 ? ma_format_s16 : ma_format_u8, m_NextChunk->waveHeader.sampleRate * 6);
        ma_sound_init_from_data_source(m_Engine, m_AudioBuffer->GetDataSource(), 0, NULL, &p_Sound);

        ma_node_attach_output_bus(&p_Sound, 0, m_Engine, 0);

        p_IsSoundInitialized = true;
    }

    if (!m_AudioBuffer->CanWrite(waveSize / (m_NextChunk->waveHeader.bitsPerSample / 8)))
    {
        return false;
    }

    ENGINE_NAMESPACE::BRSTM::WAVEData waveData = m_NextChunk->get_samples();

    m_AudioBuffer->Queue(waveData.data + waveOffset, waveSize / (m_NextChunk->waveHeader.bitsPerSample / 8));

    waveData.free();
    delete m_NextChunk;
    m_NextChunk = 0;

    if (!ma_sound_is_playing(&p_Sound) || ma_sound_at_end(&p_Sound))
    {
        ma_sound_start(&p_Sound);
    }

	return true;
}
