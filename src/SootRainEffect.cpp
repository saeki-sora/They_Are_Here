#include "pch.h"
#include "SootRainEffect.h"
#include "VisualObject.h"

using namespace DirectX::SimpleMath;



SootRainEffect::SootRainEffect(int screenW, int screenH, const std::string& texPath, int maxParticles)
    : m_ScreenW(screenW)
    , m_ScreenH(screenH)
    , m_TexPath(texPath)
    , m_Max(std::max(1, maxParticles))
{
    std::random_device rd;
    m_Rng = std::mt19937(rd());

    // 粒子プール（固定長で安定）
    m_Particles.resize(m_Max);

    // 1枚クアッドを生成して使い回す
    m_Quad = std::make_unique<VisualObject>();
    m_Quad->Init();
    m_Quad->SetTexture(m_TexPath.c_str());

    // 初期状態から漂っている雰囲気にする
    for (auto& p : m_Particles)
    {
        Respawn(p);

        const float halfW = static_cast<float>(m_ScreenW) * 0.5f;
        const float halfH = static_cast<float>(m_ScreenH) * 0.5f;

        // 画面内＋少し外まで散らす（中心原点）
        p.pos.x = RandF(-halfW - 80.0f, halfW + 80.0f);
        p.pos.y = RandF(-halfH - 80.0f, halfH + 80.0f);
    }

    m_SpawnTimer = 0.0f;
    m_NextSpawn = RandF(0.06f, 0.20f);
}

SootRainEffect::~SootRainEffect()
{
    if (m_Quad)
    {
        m_Quad->Uninit();
    }
}

void SootRainEffect::Stop()
{
    if (!m_Playing) return;
    m_Playing = false;

    if (m_onComplete) m_onComplete();
}

float SootRainEffect::RandF(float a, float b)
{
    std::uniform_real_distribution<float> dist(a, b);
    return dist(m_Rng);
}

int SootRainEffect::RandI(int a, int b)
{
    std::uniform_int_distribution<int> dist(a, b);
    return dist(m_Rng);
}

bool SootRainEffect::RandChance(float p)
{
    std::bernoulli_distribution dist(p);
    return dist(m_Rng);
}


void SootRainEffect::Respawn(Particle& p, bool forceNear)
{
    // 近景レイヤー（大きめ＆少し速い）を控えめに混ぜる
    p.nearLayer = forceNear ? true : RandChance(0.08f);

    // Renderer::SetWorldViewProjection2D() は Application::GetWidth/Height 基準で投影しているため、
    // Effect側も同じスクリーンサイズ基準で範囲を作る（ズレると途中で消える原因になる）
    const float halfW = static_cast<float>(m_ScreenW) * 0.5f;
    const float halfH = static_cast<float>(m_ScreenH) * 0.5f;

    // 画面上から生成（中心原点：xは -halfW～+halfW）
    p.pos.x = RandF(-halfW - 80.0f, halfW + 80.0f);
    p.pos.y = halfH + RandF(60.0f, 320.0f); // 上端より少し上
    p.pos.z = 0.0f;

    // 落下速度（煤は遅め。近景だけ少し速く）
    const float fallBase = p.nearLayer ? 62.0f : 36.0f;
    p.vel.x = RandF(-14.0f, 14.0f);
    p.vel.y = -fallBase * RandF(0.85f, 1.15f);
    p.vel.z = 0.0f;

    // 回転（主張しすぎない）
    p.rotZ = RandF(-3.1415f, 3.1415f);
    p.angVel = RandF(-0.6f, 0.6f);

    // 漂い（大きすぎると横に飛びすぎるので控えめに）
    p.swayAmp = RandF(30.0f, 70.0f);
    p.swayFreq = RandF(0.7f, 1.5f);
    p.phase = RandF(0.0f, 6.28f);

    // サイズ：全体的に細かく、たまにデカい事故を出さない
    if (p.nearLayer)
    {
        p.scale = RandF(0.75f, 1.00f);
    }
    else
    {
        p.scale = RandF(0.55f, 0.85f);
    }
    p.scale = std::min(p.scale, 1.00f);

    // 寿命：下端まで落ち切る時間をベースに自動計算（途中消え防止）
    // 「十分に画面外へ抜ける」下端（中心原点）
    const float bottomY = -halfH - 260.0f;

    // 落下距離（上→下）
    const float travel = (p.pos.y - bottomY);

    // 落下速度（vel.y は負なので正に直して計算）
    const float fallSpeed = std::max(1.0f, -p.vel.y);

    // 下まで落ちるのに必要な最短時間
    const float minLife = travel / fallSpeed;

    // 余韻：少しだけランダムを足して“パラパラ感”を維持
    p.maxLife = minLife + RandF(0.6f, 1.8f);
    p.life = p.maxLife;
}



void SootRainEffect::Update(float deltaTime)
{
    if (!m_Playing) return;

    // ワープ防止（演出として破綻しないため）
    deltaTime = std::min(deltaTime, 0.05f);

    // パラパラ感：ランダム間隔で数個を上から追加する
    m_SpawnTimer += deltaTime;
    if (m_SpawnTimer >= m_NextSpawn)
    {
        m_SpawnTimer = 0.0f;
        m_NextSpawn = RandF(0.06f, 0.20f);

        const int burst = RandI(1, 2);
        for (int i = 0; i < burst; ++i)
        {
            const int idx = RandI(0, m_Max - 1);
            Respawn(m_Particles[idx]);
        }
    }

    // 粒子更新
    for (auto& p : m_Particles)
    {
        p.life -= deltaTime;

        const float age = (p.maxLife - p.life);
        const float sway = std::sin(age * p.swayFreq + p.phase) * p.swayAmp;

        p.pos.x += (p.vel.x + sway * 0.15f) * deltaTime;
        p.pos.y += p.vel.y * deltaTime;

        p.rotZ += p.angVel * deltaTime;

        // 画面外 or 寿命で再スポーン
        const float halfW = static_cast<float>(m_ScreenW) * 0.5f;
        const float halfH = static_cast<float>(m_ScreenH) * 0.5f;

        const float marginX = 800.0f;
        const float marginTop = 800.0f;
        const float marginBottom = 1200.0f;

        const bool outBottom = (p.pos.y < -halfH - marginBottom);
        const bool outTop = (p.pos.y > halfH + marginTop);
        const bool outSide = (p.pos.x < -halfW - marginX) || (p.pos.x > halfW + marginX);

        if (p.life <= 0.0f || outBottom || outTop || outSide)
        {
            Respawn(p);
        }
    }
}

void SootRainEffect::Draw()
{
    if (!m_Playing) return;
    if (!m_Quad) return;

    for (const auto& p : m_Particles)
    {
        const float baseSize = p.nearLayer ? 17.0f : 7.0f;
        const float s = baseSize * p.scale;

        m_Quad->SetPosition(p.pos.x, p.pos.y, p.pos.z);
        m_Quad->SetScale(s, s, 1.0f);

        m_Quad->SetRotation(Vector3(0.0f, 0.0f, p.rotZ));

        m_Quad->Draw();
    }
}

