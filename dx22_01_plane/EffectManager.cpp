#include "EffectManager.h"

EffectManager& EffectManager::GetInstance()
{
	static EffectManager instance;
	return instance;
}

void EffectManager::Init()
{
}

void EffectManager::Uninit()
{
	m_activeEffect.reset();
}

void EffectManager::Update()
{
	if (m_activeEffect)
	{
		m_activeEffect->Update();

		// エフェクトが終わったら自動で破棄する
		if (!m_activeEffect->IsPlaying())
		{
			m_activeEffect.reset();
		}
	}
}

void EffectManager::Draw()
{
	if (m_activeEffect)
	{
		m_activeEffect->Draw();
	}
}

bool EffectManager::IsPlaying() const
{
	// m_activeEffectがnullptrでなく、かつ再生中ならtrue
	return m_activeEffect && m_activeEffect->IsPlaying();
}

//アクティブなエフェクトが存在し、そのエフェクトがブロックすべきと判断したらtrue
bool EffectManager::ShouldBlockUpdate() const
{

	if (m_activeEffect)
	{
		return m_activeEffect->ShouldBlockUpdate();
	}
	
	return false;
}