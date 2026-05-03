#pragma once
#include "Shader.h"
#include"ShaderManager.h"
#include "Camera.h"
#include"Renderer.h"
#include "MeshRenderer.h"
#include "Texture.h"
#include "Material.h"
#include "SimpleBoxCollider.h"

class Object {

protected:

	//姿勢情報
	DirectX::SimpleMath::Vector3 m_Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_Rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);

	float m_TargetYaw = 0.0f; //旋回の目標角度

	// 描画の為の情報（見た目に関わる部分）
	Shader m_Shader; // シェーダー
	std::vector<std::unique_ptr<Material>> m_Materiales;//マテリアル情報
	std::vector<SUBSET> m_subsets;//サブセット情報
	std::vector<std::shared_ptr<Texture>> m_Textures;//テクスチャ

	// 描画の為の情報（メッシュに関わる情報）
	MeshRenderer m_MeshRenderer; // 頂点バッファ・インデックスバッファ・インデックス数

	bool m_IsPendingDestroy = false; // 破棄フラグ

public:

	Object(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& scale);//コンストラクタ
	virtual ~Object();//デストラクタ
	virtual void Init();
	virtual void Update(float deltaTime);
	virtual void Draw();
	virtual void DrawShadow() {}  // シャドウマップ描画（デフォルトは何もしない）
	virtual void Uninit();

	//セッター
	virtual void SetPosition(const DirectX::SimpleMath::Vector3& pos);//ポジション
	virtual void SetScale(const DirectX::SimpleMath::Vector3& scale);//スケール
	virtual void SetRotation(const DirectX::SimpleMath::Vector3& rot);//回転
	void SetTargetYaw(float yaw) { m_TargetYaw = yaw; }// 旋回の目標角度を設定

	//ゲッター
	virtual DirectX::SimpleMath::Vector3 GetPosition()const;//ポジション
	virtual DirectX::SimpleMath::Vector3 GetScale()const;//スケール
	virtual DirectX::SimpleMath::Vector3 GetRotation()const;//回転
	float GetTargetYaw() const; // 旋回の目標角度を取得

	virtual bool Is3D() const { return true; } //オブジェクトタイプ。デフォルトは3D

	virtual void Destroy() { m_IsPendingDestroy = true; }// オブジェクトを破棄する (フラグを立てる)
	bool IsPendingDestroy() const { return m_IsPendingDestroy; }// 破棄フラグが立っているか

};

