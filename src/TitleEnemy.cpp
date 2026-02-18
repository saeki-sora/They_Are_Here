#include "pch.h"
#include "TitleEnemy.h"
#include "StaticMesh.h"


TitleEnemy::TitleEnemy(
	const DirectX::SimpleMath::Vector3& pos,
	const DirectX::SimpleMath::Vector3& size) :
	ColliderObject(pos, size)
{
}

TitleEnemy::~TitleEnemy()
{
}

void TitleEnemy::Init()
{
	StaticMesh staticmesh;

	std::u8string modelFile = u8"assets/model/enemy/Enemy_Mid.fbx";
	std::string texDirectory = "assets/model/enemy";

	std::string tmp(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
	staticmesh.Load(tmp, texDirectory);

	m_MeshRenderer.Init(staticmesh);

	m_subsets = staticmesh.GetSubsets();
	m_Textures = staticmesh.GetTextures();

	auto materials = staticmesh.GetMaterials();
	for (size_t i = 0; i < materials.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Create(materials[i]);
		m_Materiales.push_back(std::move(mat));
	}

	m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");
}

void TitleEnemy::StartScareSequence(const Vector3& cameraPos)
{
	m_TargetPos = cameraPos;
	m_State = State::Turning; // まずは振り返りモードへ

	// パラメータ初期化
	m_SwaySpeed = 2.0f;   // 振り返り中はゆっくり不気味に揺れる
	m_SwayAmount = 0.2f;
	m_BobAmount = 0.1f;
}

void TitleEnemy::Update(float deltaTime)
{
	m_Timer += deltaTime;

	if (m_State == State::Turning)
	{
		//ゆっくり振り返るフェーズ

		Vector3 diff = m_TargetPos - m_Position;

		// 水平回転
		float targetYaw = atan2(diff.x, diff.z);
		targetYaw += 3.14159f; // 背中補正

		// 角度差分
		float diffYaw = targetYaw - m_Rotation.y;
		while (diffYaw > 3.14159f) diffYaw -= 6.28318f;
		while (diffYaw < -3.14159f) diffYaw += 6.28318f;

		// 回転適用
		float rotateAmount = diffYaw * m_TurnSpeed * deltaTime;
		float minSpeed = 1.0f * deltaTime;
		if (abs(rotateAmount) < minSpeed) rotateAmount = (diffYaw > 0 ? minSpeed : -minSpeed);
		m_Rotation.y += rotateAmount;


		// 垂直回転
		float targetPitch = 0.25f; // 約15度くらい前に傾ける (威圧感)

		// Pitchもゆっくり補間する
		float diffPitch = targetPitch - m_Rotation.x;
		m_Rotation.x += diffPitch * m_TurnSpeed * deltaTime;


		// 完了判定
		if (abs(diffYaw) < 0.02f)
		{
			m_State = State::Dashing;

			// ズレ補正
			m_Rotation.y = targetYaw;
			m_Rotation.x = targetPitch; // 固定値に合わせる

			// パラメータ切り替え
			m_SwaySpeed = 30.0f;
			m_SwayAmount = 1.0f;
			m_BobAmount = 0.5f;
		}
	}
	else if (m_State == State::Dashing)
	{
		// 猛ダッシュフェーズ

		Vector3 diff = m_TargetPos - m_Position;

		// Yawは常にカメラを追う
		float targetYaw = atan2(diff.x, diff.z);
		targetYaw += 3.14159f;
		m_Rotation.y = targetYaw;

		// Pitchは固定の前傾姿勢をキープ
		m_Rotation.x = 0.25f; 

		// 前進
		diff.y = 0;
		if (diff.LengthSquared() > 0.001f) diff.Normalize();
		m_Position += diff * m_DashSpeed * deltaTime;
	}
}

void TitleEnemy::Draw()
{
	// 揺れ計算
	Vector3 drawPos = m_Position;

	if (m_State != State::Idle)
	{
		Matrix rotMat = Matrix::CreateRotationY(m_Rotation.y);
		Vector3 right = Vector3::Transform(Vector3::Right, rotMat);

		// 左右揺れ
		float sway = sin(m_Timer * m_SwaySpeed) * m_SwayAmount;
		drawPos += right * sway;

		// 縦揺れ
		float bob = abs(sin(m_Timer * m_SwaySpeed)) * m_BobAmount;
		drawPos.y += bob;
	}

	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(drawPos); // 揺らした座標を使う
	Matrix s = Matrix::CreateScale(m_Scale);
	Matrix world = s * r * t;

	Renderer::SetWorldMatrix(&world);

	m_Shader.SetGPU();
	m_MeshRenderer.BeforeDraw();
	for (size_t i = 0; i < m_subsets.size(); ++i)
	{
		int matIdx = m_subsets[i].MaterialIdx;
		m_Materiales[matIdx]->SetGPU();
		if (m_Materiales[matIdx]->isTextureEnable()) m_Textures[matIdx]->SetGPU();
		m_MeshRenderer.DrawSubset(m_subsets[i].IndexNum, m_subsets[i].IndexBase, m_subsets[i].VertexBase);
	}
}


void TitleEnemy::Uninit()
{
}

