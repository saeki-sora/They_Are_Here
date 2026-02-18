#pragma once

#include "Camera.h"
#include "Input.h"
#include "DebugRenderer.h"

using namespace DirectX::SimpleMath;

//繧ｷ繝ｳ繧ｰ繝ｫ繝医Φ繝代ち繝ｼ繝ｳ縺ｧ繧ｲ繝ｼ繝繧ｯ繝ｩ繧ｹ繧貞ｮ夂ｾｩ
class Game
{
private:
	Game(); // 繧ｳ繝ｳ繧ｹ繝医Λ繧ｯ繧ｿ
	~Game(); // 繝・せ繝医Λ繧ｯ繧ｿ

	std::unique_ptr<Input> m_Input;  // 蜈･蜉帛・逅・
	std::unique_ptr<Camera> m_MainCamera; // 繧ｫ繝｡繝ｩ

public:
	static Game& GetInstance();

	// 繧ｳ繝斐・遖∵ｭ｢
	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	void Init();
	void Update(float); 
	void Draw();
	void Uninit();

	Camera& GetMainCamera() { return *m_MainCamera; } // 繧ｫ繝｡繝ｩ蜿門ｾ・
};
