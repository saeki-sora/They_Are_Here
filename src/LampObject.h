#pragma once
#include "Object.h"

// ランプオブジェクトクラス
class LampObject : public Object
{
public:
	LampObject(
		const DirectX::SimpleMath::Vector3& pos,
		const DirectX::SimpleMath::Vector3& faceDir,
		float range,
		const DirectX::SimpleMath::Color& color);

	~LampObject() = default;

	void Init() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void DrawShadow() override;
	void Uninit() override;

private:
	DirectX::SimpleMath::Vector3 m_faceDir;
	float                        m_range;
	DirectX::SimpleMath::Color   m_color;
	int                          m_lightId = -1;

	// ライトのちらつき用
	float m_time  = 0.0f;
	float m_phase = 0.0f;// ランプの明るさの変化の振幅


	 static MeshRenderer                           s_SharedRenderer;
	 static bool                                   s_MeshReady;
	 static DirectX::SimpleMath::Vector3           s_ColliderExtents;
	 static std::vector<SUBSET>                    s_SharedSubsets;
	 static std::vector<MATERIAL>                  s_SharedMaterials;
	 static std::vector<std::shared_ptr<Texture>>  s_NormalTextures;
};
