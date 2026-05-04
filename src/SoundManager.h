#pragma once
#include <Audio.h>
#include <map>
#include <string>
#include <memory>

using namespace DirectX;

class SoundManager
{
public:
    // シングルトン化
    static SoundManager& GetInstance() {
        static SoundManager instance;
        return instance;
    }

    void Init();
    void Uninit();
    void Update(float deltaTime);

    void LoadSound(const std::string& tag, const wchar_t* filePath);
    void PlaySE(const std::string& tag);
    void PlayBGM(const std::string& tag, bool loop = true);
    void StopBGM();
    void FadeBGMOut(float duration); // BGMをフェードアウトして停止
    void PauseBGM();
    void ResumeBGM();

private:
    SoundManager();
    ~SoundManager();

    std::unique_ptr<AudioEngine> m_audioEngine;
    std::map<std::string, std::unique_ptr<SoundEffect>> m_soundEffects;
    std::unique_ptr<SoundEffectInstance> m_currentBGM;

    bool  m_IsFadingOut    = false; // フェードアウト中フラグ
    float m_FadeOutDuration = 1.0f; // フェード総時間
    float m_FadeOutElapsed  = 0.0f; // 経過時間
};