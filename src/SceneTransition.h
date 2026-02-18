#pragma once

// ================================================================================
// シーントランジション基底クラス
// ================================================================================
class SceneTransition {
public:
	enum class Phase {
		FadeOut,
		Loading,
		FadeIn
	};

	virtual ~SceneTransition() = default;

	virtual void Start() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;

	virtual bool IsComplete() const = 0;
	virtual Phase GetPhase() const = 0;
	virtual float GetProgress() const = 0;
};