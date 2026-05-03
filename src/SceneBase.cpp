#include "pch.h"
#include "SceneBase.h"

// ================================================================================
// シーンの基本的な実装
// ================================================================================

SceneBase::~SceneBase() 
{
    ClearSceneObjects();
}

// デフォルトのライフサイクル実装
void SceneBase::OnLoad() 
{
    
    m_LoadProgress = 1.0f;
}


// 派生クラスで必要に応じてオーバーライド
void SceneBase::OnInit() 
{
    
    m_IsInitialized = true;
}


// 派生クラスで必要に応じてオーバーライド
void SceneBase::OnActivate() 
{
    
}


// フレームごとの更新処理
void SceneBase::Update(float deltaTime)
{
    OnUpdate(deltaTime);

	// オブジェクトの更新と破棄処理
    for (auto it = m_SceneObjects.begin(); it != m_SceneObjects.end(); )
    {
        auto& obj = *it;

        if (obj->IsPendingDestroy())
        {
            obj->Uninit();
            it = m_SceneObjects.erase(it);
        }
        else
        {
            if (!m_IsGamePaused) // ポーズ中はオブジェクト更新をスキップ
            {
                obj->Update(deltaTime);
            }
            ++it;
        }
    }
}

// 描画処理
void SceneBase::Draw()
{

    OnDraw();

    
    for (auto& obj : m_SceneObjects)
    {
        if (obj->Is3D())
        {
            obj->Draw();
        }
    }
}

// UI描画処理
void SceneBase::DrawUI()
{
    OnDrawUI();

    for (auto& obj : m_SceneObjects)
    {
        if (!obj->Is3D())
        {
            obj->Draw();
        }
    }
}


// エフェクトより上に描画するUI
void SceneBase::DrawOverlayUI()
{
    OnDrawOverlayUI();
}


// シャドウマップ描画処理
void SceneBase::DrawShadow()
{
    // 3Dオブジェクトのシャドウ描画
    for (auto& obj : m_SceneObjects)
    {
        if (obj->Is3D() && !obj->IsPendingDestroy())
        {
            obj->DrawShadow();
        }
    }
}


// 派生クラスで必要に応じてオーバーライド
void SceneBase::OnDeactivate() {
    
}


// 派生クラスで必要に応じてオーバーライド
void SceneBase::OnUnload()
{
    ClearSceneObjects();
    m_IsInitialized = false;
}


// オブジェクトを削除する
void SceneBase::DeleteObject(std::weak_ptr<Object> weak_pt)
{
    if (auto shared_pt = weak_pt.lock())
    {
        auto it = std::find(m_SceneObjects.begin(), m_SceneObjects.end(), shared_pt);
        if (it != m_SceneObjects.end())
        {
            (*it)->Uninit();
            m_SceneObjects.erase(it);
        }
    }
}


// シーン内のすべてのオブジェクトを削除する
void SceneBase::ClearSceneObjects() 
{
    for (auto& obj : m_SceneObjects) {
        obj->Uninit();
    }
    m_SceneObjects.clear();
}