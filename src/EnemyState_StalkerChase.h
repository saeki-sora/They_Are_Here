#pragma once
#include "EnemyState.h"

class EnemyState_StalkerChase : public EnemyState
{
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, float deltaTime) override;
    void Exit(Enemy* enemy) override;

private:
    float m_PathUpdateTimer = 0.0f; // パス再計算用のタイマー

    //視線が切れてからの経過時間
    float m_TimeSinceLostSight = 0.0f;

    //何秒間壁の裏に隠れ続けたら諦めるか
    const float GIVE_UP_TIME = 15.0f;
};