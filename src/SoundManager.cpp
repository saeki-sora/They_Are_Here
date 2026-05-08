#include "pch.h"
#include "SoundManager.h"


SoundManager::SoundManager() {}

SoundManager::~SoundManager() 
{
    Uninit(); 
}

void SoundManager::Init()
{
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;

#ifdef _DEBUG
    eflags |= AudioEngine_Debug;
#endif

    m_audioEngine = std::make_unique<AudioEngine>(eflags);
}



void SoundManager::Uninit()
{
    if (m_audioEngine)
    {
        m_audioEngine->Suspend();
    }
    m_currentBGM.reset();
    m_soundEffects.clear();
    m_audioEngine.reset();
}


void SoundManager::Update(float deltaTime)
{
    if (m_audioEngine)
    {
        if (!m_audioEngine->Update())
        {
			// オーディオデバイスロスト時の復帰処理をここに追加可能
        }
    }

    // BGMフェードアウト処理
    if (m_IsFadingOut && m_currentBGM)
    {
        m_FadeOutElapsed += deltaTime;
        float t = std::min(m_FadeOutElapsed / m_FadeOutDuration, 1.0f);
        m_currentBGM->SetVolume(1.0f - t);
        if (t >= 1.0f)
        {
            m_currentBGM->Stop();
            m_IsFadingOut = false;
        }
    }
}



void SoundManager::LoadSound(const std::string& tag, const wchar_t* filePath)
{
    if (m_audioEngine) m_soundEffects[tag] = std::make_unique<SoundEffect>(m_audioEngine.get(), filePath);
}



void SoundManager::PlaySE(const std::string& tag) 
{
    if (m_soundEffects.find(tag) != m_soundEffects.end())
    {
        m_soundEffects[tag]->Play();
    }
}



void SoundManager::PlayBGM(const std::string& tag, bool loop)
{
    if (m_soundEffects.find(tag) == m_soundEffects.end()) return;

    StopBGM(); // 前のBGMを止める

    m_currentBGM = m_soundEffects[tag]->CreateInstance();
    m_currentBGM->SetVolume(1.0f);
    m_currentBGM->Play(loop);
}



void SoundManager::StopBGM()
{
    m_IsFadingOut = false;
    if (m_currentBGM)
    {
        m_currentBGM->Stop();
        m_currentBGM.reset();
    }
}



void SoundManager::FadeBGMOut(float duration)
{
    if (!m_currentBGM) return;
    m_IsFadingOut     = true;
    m_FadeOutDuration = duration;
    m_FadeOutElapsed  = 0.0f;
    m_currentBGM->SetVolume(1.0f);
}



void SoundManager::SetBGMVolume(float volume)
{
    if (m_currentBGM) m_currentBGM->SetVolume(volume);
}



void SoundManager::PauseBGM()
{
    if (m_currentBGM) m_currentBGM->Pause();
}



void SoundManager::ResumeBGM()
{
    if (m_currentBGM) m_currentBGM->Resume();
}