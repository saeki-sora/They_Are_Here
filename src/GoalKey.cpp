#include "pch.h"
#include "GoalKey.h"
#include "SceneManager.h"
#include "Player.h"
#include "Enemy.h"
#include "Renderer.h"
#include "StaticMesh.h"
#include "DebugManager.h"
#include "Game.h"
#include "SoundManager.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

GoalKey::GoalKey(const Vector3& pos, const Vector3& size)
    : ColliderObject(pos, size)
{
}

GoalKey::~GoalKey() {}


void GoalKey::Init()
{
    //メッシュとテクスチャの読み込み
    StaticMesh staticmesh;

    // 3Dモデルファイル
    std::u8string modelFile = u8"assets/model/gole_key/Goal_Key.fbx";

    // テクスチャディレクトリ
    std::string texDirectory = "assets/model/gole_key";

    // Meshを読み込む
    std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
    staticmesh.Load(tmpStr1, texDirectory);

    m_MeshRenderer.Init(staticmesh);

    //当たり判定用のサイズを取得
    collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());

    // シェーダオブジェクト
    m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    // サブセット情報の取得
    m_subsets = staticmesh.GetSubsets();

    // テクスチャ情報の取得
    m_Textures = staticmesh.GetTextures();

    // マテリアル情報の取得
    std::vector<MATERIAL> materials = staticmesh.GetMaterials();

    // マテリアル数分ループ
    for (int i = 0; i < materials.size(); ++i)
    {
        // マテリアルオブジェクト生成
        std::unique_ptr<Material> m = std::make_unique<Material>();

        // マテリアル情報をセット
        m->Create(materials[i]);

        // マテリアルオブジェクトを配列に追加
        m_Materiales.push_back(std::move(m));
    }

	m_Rotation.x = XMConvertToRadians(90.0f); // X軸に90度回転させて立たせる
}



void GoalKey::Update(float deltaTime)
{
    // プレイヤーの遅延キャッシュ（初回のみ）
    if (m_CachedPlayer.expired())
    {
        m_CachedPlayer = SceneManager::GetInstance().FindObject<Player>();
    }

    // ターゲットへの追従
    FollowTarget(deltaTime);

    // コライダー位置と回転更新
    collider.center = m_Position;
    collider.rotation = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);

    // プレイヤーとの当たり判定（まだ取得していない場合のみ）
    if (!m_IsObtained)
    {
        CheckCollisionWithPlayer();
    }
}



void GoalKey::Draw()
{
    // SRT行列の作成
    Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
    Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
    Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

    Matrix worldmtx;
    worldmtx = s * r * t;
    Renderer::SetWorldMatrix(&worldmtx); // GPUにセット
    m_Shader.SetGPU();

    // インデックスバッファ・頂点バッファをセット
    m_MeshRenderer.BeforeDraw();

    // マテリアル数分ループ
    for (int i = 0; i < m_subsets.size(); ++i)
    {
        // マテリアルをセット
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

    if (DebugManager::GetInstance().ShouldShowColliders()) { collider.DrawDebugCollider(Game::GetInstance().GetMainCamera()); }

    //	// スケールを含めない行列を渡して描画します。

}




void GoalKey::Uninit()
{
}




void GoalKey::SetTarget(std::weak_ptr<ColliderObject> target)
{
    m_Target = target;
}





void GoalKey::FollowTarget(float deltaTime)
{
    if (auto target = m_Target.lock())
    {
        // ターゲットの位置と回転を取得
        Vector3 targetPos = target->GetPosition();
        Vector3 targetRot = Vector3::Zero;

		// ターゲットがEnemyなら回転を取得（Playerは回転を考慮しない）
        if (auto enemy = std::dynamic_pointer_cast<Enemy>(target))
        {
            targetRot = enemy->GetRotation();
        }

        // --- 位置計算 背中に配置 ---
        // Y軸回転から後ろ方向ベクトルを作成
        Vector3 forward;
        forward.x = std::sin(targetRot.y);
        forward.y = 0.0f;
        forward.z = std::cos(targetRot.y);
        forward.Normalize();

        Vector3 backward = -forward;

        // 目標位置 = ターゲット位置 + (後ろ方向 * 距離) + 高さ調整
        Vector3 desiredPos = targetPos + (backward * FOLLOW_DISTANCE);
        desiredPos.y = targetPos.y + HEIGHT_OFFSET;

        // ふわふわさせる演出
         //desiredPos.y += std::sin(Game::GetInstance().GetTotalTime() * 2.0f) * 5.0f;


        // 現在位置から目標位置へ滑らかに移動(Lerp)
        m_Position = Vector3::Lerp(m_Position, desiredPos, LERP_FACTOR);

        // 回転もターゲットに合わせる（常に背中が見えるように）
        m_Rotation = targetRot;
        m_Rotation.x = XMConvertToRadians(90.0f); // X軸の立ち姿勢を維持
    }
}

void GoalKey::CheckCollisionWithPlayer()
{
    if (auto player = m_CachedPlayer.lock())
    {
        if (this->collider.CheckCollision(player->GetCollider()))
        {
            m_IsObtained = true;
            player->SetHasKey(true);
            SoundManager::GetInstance().PlaySE(SoundTag::SE_KeyPickup);
            this->Destroy();
#ifdef _DEBUG
            std::cout << "Key Obtained! Follow Target Changed to Player." << std::endl;
#endif
        }
    }
}