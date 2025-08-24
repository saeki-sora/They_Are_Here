#pragma once

class Enemy;

// 敵AIの全状態の基底クラス
// 各敵は現在の状態へのポインタを保持し、行動を状態に委譲する
// 新しい行動を追加するには、EnemyStateから継承してEnter、Update、Exitをオーバーライドする
// 状態オブジェクトの生存期間はEnemyクラス内でstd::unique_ptrによって管理される
class EnemyState
{
public:
    virtual ~EnemyState() = default;
    
    virtual void Enter(Enemy* enemy) {}// 状態に入るときに呼ばれる。初期化処理をここに実装する

    virtual void Update(Enemy* enemy, float deltaTime) = 0;

    virtual void Exit(Enemy* enemy) {}// 状態が終了するときに呼ばれる
};