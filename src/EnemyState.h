#pragma once

class Enemy;


class EnemyState
{
public:
    virtual ~EnemyState() = default;
    
    virtual void Enter(Enemy* enemy) {}// 状態に入るときに呼ばれる。初期化処理をここに実装する

    virtual void Update(Enemy* enemy, float deltaTime) = 0;

    virtual void Exit(Enemy* enemy) {}// 状態が終了するときに呼ばれる

    virtual bool IsChaseState() const { return false; }// 追跡状態かどうか
};