#pragma once
#include "ColliderObject.h"
#include"VisualObject.h"

class SpeedUpItem : public ColliderObject
{
private:

    int m_SpeedUpTime;//高速化効果時間

public:

    SpeedUpItem(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& size);
    ~SpeedUpItem() override;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Uninit() override;
};