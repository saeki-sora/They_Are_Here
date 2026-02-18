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


void SoundManager::Update()
{
    if (m_audioEngine)
    {
        if (!m_audioEngine->Update()) 
        {
			// オーディオデバイスロスト時の復帰処理をここに追加可能
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
    m_currentBGM->Play(loop);
}



void SoundManager::StopBGM() 
{
    if (m_currentBGM) {
        m_currentBGM->Stop();
        m_currentBGM.reset();
    }
}