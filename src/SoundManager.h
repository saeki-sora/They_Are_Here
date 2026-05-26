#pragma once
#include <Audio.h>
#include <map>
#include <memory>
#include "SoundTag.h"

using namespace DirectX;

class SoundManager
{
public:

    // シングルトン化
    static SoundManager& GetInstance()
    {
        static SoundManager instance;
        return instance;
    }

    void Init();
    void Uninit();
    void Update(float deltaTime);

    void LoadSound(SoundTag tag, const wchar_t* filePath);
    void PlaySE(SoundTag tag);

    void PlayBGM(SoundTag tag, bool loop = true);
    void StopBGM(SoundTag tag);
    void StopAllBGM();
    void SetBGMVolume(SoundTag tag, float volume);
	void PauseBGM(SoundTag tag);//BGMを一時停止
	void ResumeBGM(SoundTag tag);//BGMを再開
    void FadeBGMOut(SoundTag tag, float duration);

private:
    SoundManager();
    ~SoundManager();

    struct BGMFadeState { float duration; float elapsed; };

    std::unique_ptr<AudioEngine>                                 m_audioEngine;
    std::map<SoundTag, std::unique_ptr<SoundEffect>>             m_soundEffects;
    std::map<SoundTag, std::unique_ptr<SoundEffectInstance>>     m_activeBGMs;
    std::map<SoundTag, BGMFadeState>                             m_fadeStates;
};
