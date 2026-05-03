#include "pch.h"
#include "loversEnemy.h"
#include "EnemyState_loversChase.h" 
#include "MakeMap.h"
#include "Pathfinder.h"



void loversEnemy::Init()
{
    Enemy::Init();

    // ライトを黄色にする
    m_SpotLightColor = DirectX::SimpleMath::Color(1.0f, 1.0f, 0.0f, 1.0f);

    // 足の速さは通常より少し速め
    m_ChaseSpeed = 75.0f;
    m_CurrentMaxSpeed = m_SearchSpeed;

    m_IsFlankingMode = false;
}

void loversEnemy::Update(float deltaTime)
{
    // 自分がプレイヤーを見つけた場合の処理
    if (CanSeePlayer())
    {
        // 自分は追跡役（通常ルート）になる
        m_IsFlankingMode = false;

        // 相方に連絡する
        if (auto partner = m_Partner.lock())
        {
            // 相方には裏取りをさせるために位置を教える
            partner->NotifyPlayerFound(m_LastPlayerPos);
        }


        if (!dynamic_cast<EnemyState_loversChase*>(m_State.get()))
        {
            ChangeState(std::make_unique<EnemyState_loversChase>());
        }
    }

    Enemy::Update(deltaTime);
}





// 相方から呼ばれる関数
void loversEnemy::NotifyPlayerFound(const DirectX::SimpleMath::Vector3& playerPos)
{
    // 相方の報告を聞いて、裏取りモードになる
    if (CanSeePlayer()) return; // 自分も見えてるなら、自分が追う

    m_IsFlankingMode = true;
    m_SharedTargetPos = playerPos;

    //現在の探索を打ち切って、即座に連携追跡ステートへ移行
    if (!dynamic_cast<EnemyState_loversChase*>(m_State.get()))
    {
        // 現在の探索を打ち切って、連携追跡ステートへ移行
        ChangeState(std::make_unique<EnemyState_loversChase>());
    }
}





// パス計算
bool loversEnemy::ComputePathTo(const DirectX::SimpleMath::Vector3& target)
{

    if (!m_Map) return false; // マップが設定されていない場合は失敗

    // --- 通常モード: 親クラスと同じ標準的なパス計算 ---
    if (!m_IsFlankingMode)
    {
        return Enemy::ComputePathTo(target);
    }

    // --- 裏取りモード: 相方のパスを避ける重み付きパス計算 ---

    // 相方が存在しない場合は通常のパス計算にフォールバック
    auto partner = m_Partner.lock();
    if (!partner)
    {
        return Enemy::ComputePathTo(target);
    }

    // 相方のルート（ウェイポイントリスト）を取得
    const auto& partnerPath = partner->GetWaypoints();

    // コストマップを作成
    std::unordered_map<int, float> costMap;

    // マップサイズはランタイムで難易度によって変わるため、MakeMap から取得する
    const int maxY = m_Map->GetSizeY();

    // 相方が通過する予定のすべてのセルをコストマップに登録
    for (const auto& pos : partnerPath)
    {
        GridCoord cell = Pathfinder::WorldToGrid(m_Map, pos);
		costMap[cell.x * maxY + cell.y] = 20.0f;//コストを高く設定して相方のルートを避けるようにする
    }

    // 相方の現在位置も避けるべきセルとして登録
    GridCoord partnerPos = Pathfinder::WorldToGrid(m_Map, partner->GetPosition());
    costMap[partnerPos.x * maxY + partnerPos.y] = 20.0f;

    // 重み付きA*アルゴリズムでパスを計算
    GridCoord start = Pathfinder::WorldToGrid(m_Map, m_Position);
    GridCoord goal = Pathfinder::WorldToGrid(m_Map, target);

    auto gridPath = Pathfinder::FindPathWithWeights(m_Map, start, goal, costMap);

    // パスが見つからなかった場合は失敗
    if (gridPath.empty())
    {
        return false;
    }

    // グリッド座標のパスをワールド座標に変換
    std::vector<DirectX::SimpleMath::Vector3> worldPath;
    worldPath.reserve(gridPath.size()); // メモリの再確保を避けるため事前に容量を確保

    for (const auto& cell : gridPath)
    {
        worldPath.push_back(Pathfinder::GridToWorld(m_Map, cell));
    }

    // 親クラスの共通処理関数を呼び出し
    ProcessPath(worldPath);

    // パス計算成功
    return !m_Waypoints.empty();
}