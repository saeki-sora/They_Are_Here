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
	// オーディオエンジンを一時停止してから全てのサウンドを停止
	if (m_audioEngine)
	{
		m_audioEngine->Suspend();
	}
	StopAllBGM();
	m_soundEffects.clear();
	m_audioEngine.reset();
}



void SoundManager::Update(float deltaTime)
{

	if (m_audioEngine)
	{
		m_audioEngine->Update();// オーディオエンジンの更新
	}


	// 各BGMのフェードアウト処理
	std::vector<SoundTag> completed;
	for (auto& [tag, fade] : m_fadeStates)
	{
		// フェードアウトの進行を更新
		fade.elapsed += deltaTime;
		float t = std::min(fade.elapsed / fade.duration, 1.0f);
		SetBGMVolume(tag, 1.0f - t);
		if (t >= 1.0f) completed.push_back(tag);
	}

	// フェードアウトが完了したBGMを停止
	for (auto& tag : completed)
	{
		m_fadeStates.erase(tag);
		StopBGM(tag);
	}
}


// サウンドを読み込む
void SoundManager::LoadSound(SoundTag tag, const wchar_t* filePath)
{
	if (m_audioEngine)
		m_soundEffects[tag] = std::make_unique<SoundEffect>(m_audioEngine.get(), filePath);
}


// SEを再生する
void SoundManager::PlaySE(SoundTag tag)
{
	auto it = m_soundEffects.find(tag);
	if (it != m_soundEffects.end()) it->second->Play();
}



// BGMを再生する
void SoundManager::PlayBGM(SoundTag tag, bool loop)
{
	// タグに対応するサウンドが存在するか確認
	auto it = m_soundEffects.find(tag);
	if (it == m_soundEffects.end()) return;

	// 同タグが既に再生中なら一度止めてから再生
	StopBGM(tag);

	// 新しいインスタンスを作成して再生
	auto inst = it->second->CreateInstance();
	inst->SetVolume(1.0f);
	inst->Play(loop);
	m_activeBGMs[tag] = std::move(inst);
}



// BGMを停止する
void SoundManager::StopBGM(SoundTag tag)
{
	m_fadeStates.erase(tag);
	auto it = m_activeBGMs.find(tag);
	if (it == m_activeBGMs.end()) return;
	it->second->Stop();
	m_activeBGMs.erase(it);
}



// 全てのBGMを停止する
void SoundManager::StopAllBGM()
{
	m_fadeStates.clear();
	for (auto& [tag, inst] : m_activeBGMs) inst->Stop();
	m_activeBGMs.clear();
}



// BGMの音量を設定する
void SoundManager::SetBGMVolume(SoundTag tag, float volume)
{
	auto it = m_activeBGMs.find(tag);
	if (it != m_activeBGMs.end()) it->second->SetVolume(volume);
}


// BGMを一時停止する
void SoundManager::PauseBGM(SoundTag tag)
{
	auto it = m_activeBGMs.find(tag);
	if (it != m_activeBGMs.end()) it->second->Pause();
}


// BGMを再開する
void SoundManager::ResumeBGM(SoundTag tag)
{
	auto it = m_activeBGMs.find(tag);
	if (it != m_activeBGMs.end()) it->second->Resume();
}


// BGMをフェードアウトする
void SoundManager::FadeBGMOut(SoundTag tag, float duration)
{
	if (!m_activeBGMs.count(tag)) return;
	m_fadeStates[tag] = { duration, 0.0f };
}
