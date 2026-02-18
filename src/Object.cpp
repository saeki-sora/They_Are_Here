#include "pch.h"
#include "Object.h"

using namespace DirectX::SimpleMath;

Object::Object(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& scale)
	:
	m_Position(pos),
	m_Scale(scale),
m_Rotation(0, 0, 0) 
{

}


Object::~Object() {}

void Object::Init() {}

void Object::Update(float deltaTime) {}

void Object::Draw() {

	// SRT情報作成
	Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
	Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
	Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

	Matrix worldmtx;
	worldmtx = s * r * t;
	Renderer::SetWorldMatrix(&worldmtx); // GPUにセット

	m_Shader.SetGPU();

	// インデックスバッファ・頂点バッファをセット
	m_MeshRenderer.BeforeDraw();

	//マテリアル数分ループ 
	for (int i = 0; i < m_subsets.size(); ++i)
	{
		// マテリアルをセット(サブセット情報の中にあるマテリアルインデックスを使用)
		m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();

		if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
		{
			m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
		}

		m_MeshRenderer.DrawSubset(
			m_subsets[i].IndexNum, // 描画するインデックス数
			m_subsets[i].IndexBase, // 最初のインデックスバッファの位置	
			m_subsets[i].VertexBase); // 頂点バッファの最初から使用
	}
}

void Object::Uninit() {}

void Object::SetPosition(const Vector3& pos) {

	m_Position.x = pos.x;
	m_Position.y = pos.y;
	m_Position.z = pos.z;

}

Vector3 Object::GetPosition()const
{
	return m_Position;
}

void Object::SetScale(const Vector3& scale)
{
	m_Scale.x = scale.x;
	m_Scale.y = scale.y;
	m_Scale.z = scale.z;
}

Vector3 Object::GetScale()const
{
	return m_Scale;
}

void Object::SetRotation(const Vector3& rot)
{
	m_Rotation.x = rot.x;
	m_Rotation.y = rot.y;
	m_Rotation.z = rot.z;
}

Vector3 Object::GetRotation()const
{
	return m_Rotation;
}

float Object::GetTargetYaw() const
{
	return m_TargetYaw;
}