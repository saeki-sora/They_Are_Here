#include "pch.h"
#include "GoalKey.h"
#include "SceneManager.h"
#include "Player.h"
#include "Enemy.h"
#include "Renderer.h"
#include "StaticMesh.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

GoalKey::GoalKey(const Vector3& pos, const Vector3& size)
    : ColliderObject(pos, size)
{
}

GoalKey::~GoalKey() {}



void GoalKey::Init()
{
    // メチE��ュ読み込み
    StaticMesh staticmesh;

    // 3DモチE��チE�Eタ
    std::u8string modelFile = u8"assets/model/player/Player.fbx";

    // チE��スチャチE��レクトリ
    std::string texDirectory = "assets/texture/terain.png";

    // Meshを読み込む
    std::string tmpStr1(reinterpret_cast<const char*>(modelFile.c_str()), modelFile.size());
    staticmesh.Load(tmpStr1, texDirectory);

    m_MeshRenderer.Init(staticmesh);

    //当たり判定用のサイズを設宁E
    collider.size = GetScale() * (staticmesh.GetMax() - staticmesh.GetMin());

    // シェーダオブジェクト生戁E
    m_Shader.Create("shader/litTextureVS.hlsl", "shader/litTexturePS.hlsl");

    // サブセチE��惁E��取征E
    m_subsets = staticmesh.GetSubsets();

    // チE��スチャ惁E��取征E
    m_Textures = staticmesh.GetTextures();

    // マテリアル惁E��取征E
    std::vector<MATERIAL> materials = staticmesh.GetMaterials();

    // マテリアル数刁E��ーチE
    for (int i = 0; i < materials.size(); ++i)
    {
        // マテリアルオブジェクト生戁E
        std::unique_ptr<Material> m = std::make_unique<Material>();

        // マテリアル惁E��をセチE��
        m->Create(materials[i]);

        // マテリアルオブジェクトを配�Eに追加
        m_Materiales.push_back(std::move(m));
    }


    // プレイヤーのキャッシュはUpdate内で遅延取得する
    // (Init時点ではSceneManagerのcurrentSceneが未設定のため)
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

    // コライダー位置更新
    collider.center = m_Position;

    // プレイヤーとの当たり判定（まだ取得してぁE��ぁE��合�Eみ�E�E
    if (!m_IsObtained)
    {
        CheckCollisionWithPlayer();
    }
}



void GoalKey::Draw()
{
    // SRT惁E��作�E
    Matrix r = Matrix::CreateFromYawPitchRoll(m_Rotation.y, m_Rotation.x, m_Rotation.z);
    Matrix t = Matrix::CreateTranslation(m_Position.x, m_Position.y, m_Position.z);
    Matrix s = Matrix::CreateScale(m_Scale.x, m_Scale.y, m_Scale.z);

    Matrix worldmtx;
    worldmtx = s * r * t;
    Renderer::SetWorldMatrix(&worldmtx); // GPUにセチE��
    m_Shader.SetGPU();

    // インチE��クスバッファ・頂点バッファをセチE��
    m_MeshRenderer.BeforeDraw();

    //マテリアル数刁E��ーチE
    for (int i = 0; i < m_subsets.size(); ++i)
    {
        // マテリアルをセチE��
        m_Materiales[m_subsets[i].MaterialIdx]->SetGPU();

        if (m_Materiales[m_subsets[i].MaterialIdx]->isTextureEnable())
        {
            m_Textures[m_subsets[i].MaterialIdx]->SetGPU();
        }

        m_MeshRenderer.DrawSubset(
            m_subsets[i].IndexNum, // 描画するインチE��クス数
            m_subsets[i].IndexBase, // 最初�EインチE��クスバッファの位置	
            m_subsets[i].VertexBase); // 頂点バッファの最初から使用
    }

    //#ifdef _DEBUG
    //	Matrix colliderWorldMatrix = r * t;
    //	// スケールを含めなぁE���Eを渡して描画します、E
    //	collider.DrawDebugCollider(Game::GetInstance().GetMainCamera(), colliderWorldMatrix);
    //#endif
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
        // ターゲチE��の位置と回転を取征E
        Vector3 targetPos = target->GetPosition();
        Vector3 targetRot = Vector3::Zero;

        // 対象がPlayerかEnemyかで回転の取得方法を刁E��めE
        // (ColliderObjectにGetRotationがあれ�E共通化できますが、今回はキャストで対忁E
        if (auto player = std::dynamic_pointer_cast<Player>(target))
        {
            targetRot = player->GetRotation();
        }
        else if (auto enemy = std::dynamic_pointer_cast<Enemy>(target))
        {
            targetRot = enemy->GetRotation();
        }

        // --- 位置計箁E 背中に配置 ---
        // Y軸回転から後ろ方向�Eクトルを作�E
        Vector3 forward;
        forward.x = std::sin(targetRot.y);
        forward.y = 0.0f;
        forward.z = std::cos(targetRot.y);
        forward.Normalize();

        Vector3 backward = -forward;

        // 目標位置 = ターゲチE��位置 + (後ろ方吁E* 距離) + 高さ調整
        Vector3 desiredPos = targetPos + (backward * FOLLOW_DISTANCE);
        desiredPos.y = targetPos.y + HEIGHT_OFFSET;

        // ふわ�Eわさせる演�E�E�オプション�E�E
         //desiredPos.y += std::sin(Game::GetInstance().GetTotalTime() * 2.0f) * 5.0f;

        // 現在位置から目標位置へ滑らかに移勁E(Lerp)
        m_Position = Vector3::Lerp(m_Position, desiredPos, LERP_FACTOR);

        // 回転もターゲチE��に合わせる�E�常に背中が見えるよぁE���E�E
        m_Rotation = targetRot;
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
			this->Destroy();
            std::cout << "Key Obtained! Follow Target Changed to Player." << std::endl;
        }
    }
}
