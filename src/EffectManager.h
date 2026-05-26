#pragma once
#include "Effect.h"

class EffectManager
{
private:
	EffectManager() = default;
	~EffectManager() = default;

	// 現在アクティブなエフェクト
	std::vector<std::unique_ptr<Effect>> m_effects;

public:
	static EffectManager& GetInstance();

	EffectManager(const EffectManager&) = delete;
	EffectManager& operator=(const EffectManager&) = delete;

	void Init();
	void Uninit();
	void Update(float deltaTime);
	void Draw();

	// 新しいエフェクトを開始し、そのポインタを返す
	template<class T, typename... Args>
	T* StartEffect(Args&&... args)
	{
		// TがEffectを継承していることをコンパイル時にチェック
		static_assert(std::is_base_of<Effect, T>::value, "T must be a type of Effect");

		auto newEffect = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = newEffect.get();       // 生ポインタを取得
		m_effects.push_back(std::move(newEffect));// 所有権をベクターに移動

#ifdef _DEBUG
		std::cout << "[EffectManager] StartEffect returning pointer: " << ptr << std::endl;
#endif

		return ptr;                     // 生ポインタを返す
	}

	// エフェクトが再生中か
	bool IsPlaying() const;

	bool ShouldBlockUpdate() const;

};

