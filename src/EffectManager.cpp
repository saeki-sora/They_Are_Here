#include "pch.h"
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
	m_effects.clear();
}

void EffectManager::Update(float deltaTime)
{
	//全てのエフェクトを更新
	for (auto& effect : m_effects)
	{
		effect->Update(deltaTime);
	}

	//終了したエフェクトのコールバックを呼び、リストから削除
	m_effects.erase(
		std::remove_if(m_effects.begin(), m_effects.end(),
			[](const std::unique_ptr<Effect>& effect) 
			{
				if (!effect->IsPlaying())
				{
					// 削除前に完了コールバックを実行
					effect->InvokeOnComplete();
					return true;
				}
				return false;
			}),

		m_effects.end());
}

void EffectManager::Draw()
{
	for (auto& effect : m_effects)
	{
		effect->Draw();
	}
}

bool EffectManager::IsPlaying() const
{
	return !m_effects.empty();//エフェクトが一つでもあれば再生中
}

//アクティブなエフェクトが存在し、そのエフェクトがブロックすべきと判断したらtrue
bool EffectManager::ShouldBlockUpdate() const
{
	// リストが空ならブロックしない
	if (m_effects.empty())
	{
		return false;
	}

	// 全てのエフェクトをチェック
	for (const auto& effect : m_effects)
	{
		// ブロックすべきエフェクトが見つかったらtrueを返す
		if (effect->ShouldBlockUpdate())
		{
			return true; //ブロックする
		}
	}

	//どれもブロック不要ならfalse
	return false;
}