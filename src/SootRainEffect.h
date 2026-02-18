#pragma once
#include "Effect.h"


class VisualObject;

class SootRainEffect final : public Effect
{
public:

    SootRainEffect(int screenW, int screenH, const std::string& texPath, int maxParticles = 140);
    ~SootRainEffect() override;

    void Update(float deltaTime) override;
    void Draw() override;

    bool IsPlaying() const override { return m_Playing; }
    bool ShouldBlockUpdate() const override { return false; }

    void Stop();

private:
    struct Particle
    {
        DirectX::SimpleMath::Vector3 pos{};
        DirectX::SimpleMath::Vector3 vel{};
        float rotZ = 0.0f;
        float angVel = 0.0f;
        float life = 0.0f;
        float maxLife = 0.0f;

        float swayAmp = 0.0f;
        float swayFreq = 0.0f;
        float phase = 0.0f;

        float scale = 1.0f;
        bool nearLayer = false;
    };

private:

    void Respawn(Particle& p, bool forceNear = false);

    float RandF(float a, float b);
    int   RandI(int a, int b);
    bool  RandChance(float p);

private:

    bool m_Playing = true;

    int m_ScreenW = 1280;
    int m_ScreenH = 720;

    std::string m_TexPath;
    int m_Max = 140;

    std::vector<Particle> m_Particles;

    std::unique_ptr<VisualObject> m_Quad;

    std::mt19937 m_Rng;

    // “パラパラ感”を作るためのスポーンタイミング
    float m_SpawnTimer = 0.0f;
    float m_NextSpawn = 0.10f;
};

