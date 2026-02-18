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
    void Uninit(); // 終了処理を追加
    void Update();

    void LoadSound(const std::string& tag, const wchar_t* filePath);
    void PlaySE(const std::string& tag);
    void PlayBGM(const std::string& tag, bool loop = true);
    void StopBGM();
    void PauseBGM();  // 必要なら
    void ResumeBGM(); // 必要なら

private:
    SoundManager();  // コンストラクタはprivateに
    ~SoundManager();

    std::unique_ptr<AudioEngine> m_audioEngine;
    std::map<std::string, std::unique_ptr<SoundEffect>> m_soundEffects;
    std::unique_ptr<SoundEffectInstance> m_currentBGM;
};