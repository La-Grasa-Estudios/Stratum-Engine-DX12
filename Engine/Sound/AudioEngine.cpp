#include "AudioEngine.h"

using namespace ENGINE_NAMESPACE;

void AudioEngine::Init()
{
    ma_engine_config config = ma_engine_config_init();

    config.sampleRate = 44100;

    ma_engine_init(&config, &m_Engine);

    s_Instance = this;
}

void AudioEngine::Shutdown()
{

    for (int i = 0; i < MAX_SOURCES; i++)
    {
        m_Sources[i] = NULL;
    }

    ma_engine_uninit(&m_Engine);
}

void AudioEngine::Update()
{
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        if (m_Sources[i])
        {
            m_Sources[i]->UpdateSource();

            if (m_Sources[i]->Removed)
            {
                m_Sources[i] = NULL;
            }
        }
    }
}

void AudioEngine::AddSource(Ref<AudioSourceBase> source)
{
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        if (!m_Sources[i])
        {
            m_Sources[i] = source;
            return;
        }
    }
}

ma_engine* AudioEngine::GetEngine()
{
    return &m_Engine;
}
