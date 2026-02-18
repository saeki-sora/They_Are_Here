#pragma once
#include "ColliderObject.h"
#include"VisualObject.h"

class InvisibleItem : public ColliderObject
{
private:


public:

    InvisibleItem(const DirectX::SimpleMath::Vector3& pos,const DirectX::SimpleMath::Vector3& size);
    ~InvisibleItem() override;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
	void Uninit() override;
};