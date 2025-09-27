#include "SceneBase.h"
#include "Game.h"

// ================================================================================
// SceneBase実装
// ================================================================================

SceneBase::~SceneBase() 
{
    ClearSceneObjects();
}

void SceneBase::OnLoad() 
{
    // デフォルト実装: 派生クラスでオーバーライド
    m_LoadProgress = 1.0f;
}

void SceneBase::OnInit() 
{
    // デフォルト実装: 派生クラスでオーバーライド
    m_IsInitialized = true;
}

void SceneBase::OnActivate() 
{
    // デフォルト実装: 派生クラスでオーバーライド
}

void SceneBase::Update(float deltaTime)
{

    OnUpdate(deltaTime);//派生クラスの更新

    // このシーンが管理するオブジェクトを更新する
    for (auto& weakObj : m_SceneObjects)
    {
        // weak_ptr が有効か確認
        if (auto sharedObj = weakObj.lock())
        {
            sharedObj->Update();
        }
    }
}

void SceneBase::Draw()
{

    OnDraw();//派生クラスの描画

    // このシーンが管理する3Dオブジェクトを描画する
    for (auto& weakObj : m_SceneObjects)
    {
        if (auto sharedObj = weakObj.lock())
        {
            if (sharedObj->Is3D()) // 3Dオブジェクトか判定
            {
                sharedObj->Draw();
            }
        }
    }
}

void SceneBase::DrawUI()
{
    OnDrawUI();

    // このシーンが管理する2Dオブジェクト(UI)を描画する
    for (auto& weakObj : m_SceneObjects)
    {
        if (auto sharedObj = weakObj.lock())
        {
            if (!sharedObj->Is3D()) // 2Dオブジェクトか判定
            {
                sharedObj->Draw();
            }
        }
    }
}

void SceneBase::OnDeactivate() {
    // デフォルト実装: 派生クラスでオーバーライド
}

void SceneBase::OnUnload()
{
    ClearSceneObjects();
    m_IsInitialized = false;
}

void SceneBase::ClearSceneObjects() 
{
    // Game側でオブジェクトを削除
    for (auto& weakObj : m_SceneObjects) {
        Game::GetInstance().DeleteObject(weakObj);
    }
    m_SceneObjects.clear();
}