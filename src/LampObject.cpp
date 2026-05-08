#include "pch.h"
#include "LampObject.h"
#include "DebugManager.h"
#include "DebugRenderer.h"
#include "Game.h"
#include "StaticMesh.h"
#include "TextureManager.h"
#include "utility.h"

using namespace DirectX::SimpleMath;

static const Color FLAME_DIM    = { 0.9f, 0.28f, 0.04f, 1.0f };
static const Color FLAME_BRIGHT = { 1.0f, 0.85f, 0.30f, 1.0f };

static std::mt19937 s_rng{ std::random_device{}() };

MeshRenderer                           LampObject::s_SharedRenderer;
bool                                   LampObject::s_MeshReady = false;
DirectX::SimpleMath::Vector3           LampObject::s_ColliderExtents;
std::vector<SUBSET>                    LampObject::s_SharedSubsets;
std::vector<MATERIAL>                  LampObject::s_SharedMaterials;
std::vector<std::shared_ptr<Texture>>  LampObject::s_NormalTextures;

LampObject::LampObject(
	const Vector3& pos,
	const Vector3& faceDir,
	float range,
	const Color& color)
	: Object(pos, Vector3(9.0f, 9.0f, 9.0f))
	, m_faceDir(faceDir)
	, m_range(range)
	, m_color(color)
{
	std::uniform_real_distribution<float> phaseDist(0.0f, 6.2831853f);
	m_phase = phaseDist(s_rng);
}

void LampObject::Init()
{
	//モデル側の問題で、モデルの向きがZ+方向を向いているため、X軸に90度回転させる
	m_Rotation.x = DirectX::XM_PIDIV2;
	m_Rotation.y = atan2f(m_faceDir.x, m_faceDir.z);

	Vector3 lightPos = m_Position + m_faceDir * 10.0f;// ライトの位置をランプの前方に少しオフセット
	m_lightId = Renderer::AddPointLight(lightPos, m_range, m_color);

	 if (!s_MeshReady)
	 {
	     StaticMesh staticmesh;
	     std::u8string modelFile = u8"assets/model/lamp/Lamp.fbx";
	     std::string tmpStr(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	     staticmesh.Load(tmpStr, "assets/model/lamp");
	     s_SharedRenderer.Init(staticmesh);
	     s_ColliderExtents = staticmesh.GetMax() - staticmesh.GetMin();
	     s_SharedSubsets   = staticmesh.GetSubsets();
	     s_SharedMaterials = staticmesh.GetMaterials();
	     s_NormalTextures  = staticmesh.GetTextures();
	     s_MeshReady = true;
	 }
	 m_subsets  = s_SharedSubsets;
	 m_Textures = s_NormalTextures;
	 m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");
	 for (int i = 0; i < (int)s_SharedMaterials.size(); ++i)
	 {
	     MATERIAL mat = s_SharedMaterials[i];
	     mat.TextureEnable = true;
	     auto m = std::make_unique<Material>();
	     m->Create(mat);
	     m_Materiales.push_back(std::move(m));
	 }
}

void LampObject::Update(float deltaTime)
{
	if (m_lightId < 0) return;// ライトが確保されていない場合は更新しない

	m_time += deltaTime;

	// 炎っぽいちらつきの計算
	float t = m_time;
	float p = m_phase;
	float flicker = 0.80f
		+ 0.10f * std::sin(t * 6.5f  + p)
		+ 0.06f * std::sin(t * 14.2f + p * 1.7f)
		+ 0.04f * std::sin(t * 2.1f  + p * 0.5f);

	flicker = std::max(0.4f, std::min(1.0f, flicker));

	// ライトの色と範囲をちらつきに応じて変化させる
	Color c;
	c.x = FLAME_DIM.x + (FLAME_BRIGHT.x - FLAME_DIM.x) * flicker;
	c.y = FLAME_DIM.y + (FLAME_BRIGHT.y - FLAME_DIM.y) * flicker;
	c.z = FLAME_DIM.z + (FLAME_BRIGHT.z - FLAME_DIM.z) * flicker;
	c.w = 1.0f;

	Vector3 lightPos = m_Position + m_faceDir * 10.0f;

	Renderer::UpdatePointLight(m_lightId, lightPos, m_range * flicker, c);
}

void LampObject::Draw()
{
	if (s_MeshReady)
	{
		Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
		Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
		Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);
		Matrix worldmtx = s * r * t;
		Renderer::SetWorldMatrix(&worldmtx);
		m_Shader.SetGPU();
		Renderer::BindLitCommonCB();
		s_SharedRenderer.BeforeDraw();
		for (int i = 0; i < (int)m_subsets.size(); ++i)
		{
			m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();
			if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
				m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
			s_SharedRenderer.DrawSubset(
				m_subsets[i].IndexNum, m_subsets[i].IndexBase, m_subsets[i].VertexBase);
		}
	}

	if (!DebugManager::GetInstance().ShouldShowLampLights()) return;

	const float ARM         = 15.0f;
	const Color debugColor  = { 1.0f, 0.55f, 0.1f, 1.0f };
	auto camera = Game::GetInstance().GetMainCamera();

	DebugRenderer::Begin(camera);

	DebugRenderer::DrawLine(m_Position - Vector3(ARM, 0, 0), m_Position + Vector3(ARM, 0, 0), debugColor);
	DebugRenderer::DrawLine(m_Position - Vector3(0, ARM, 0), m_Position + Vector3(0, ARM, 0), debugColor);
	DebugRenderer::DrawLine(m_Position - Vector3(0, 0, ARM), m_Position + Vector3(0, 0, ARM), debugColor);

	DebugRenderer::DrawRay(m_Position, m_faceDir, ARM * 2.0f, debugColor);

	DebugRenderer::End();
}

void LampObject::DrawShadow()
{

}

void LampObject::Uninit()
{
	if (m_lightId >= 0)
	{
		Renderer::RemoveLight(m_lightId);
		m_lightId = -1;
	}
}
