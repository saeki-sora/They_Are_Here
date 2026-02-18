#pragma once

// 全てのエフェクトクラスがこのインターフェースを継承する
class Effect
{
public:
	// デストラクタは仮想デストラクタにするのがお約束
	virtual ~Effect() = default;

	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;

	// エフェクトが動作中かどうかを返す
	virtual bool IsPlaying() const = 0;

	//エフェクト再生中に、他の更新処理をブロックするか。trueでブロック
	virtual bool ShouldBlockUpdate() const { return true; }

	// エフェクトが完了したときに呼ばれるコールバックを設定
	void SetOnComplete(std::function<void()> onComplete) { m_onComplete = onComplete; }

	// 完了コールバックを実行
	void InvokeOnComplete() { if (m_onComplete) m_onComplete(); }

protected:
	// エフェクト完了時に実行する処理
	std::function<void()> m_onComplete = nullptr;
};

