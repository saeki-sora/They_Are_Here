#include "pch.h"
#include "SceneBase.h"

// ================================================================================
// SceneBaseｼﾂﾁ・
// ================================================================================

SceneBase::~SceneBase() 
{
    ClearSceneObjects();
}

void SceneBase::OnLoad() 
{
    // ･ﾇ･ﾕ･ｩ･・ﾈｼﾂﾁ・ ﾇﾉﾀｸ･ｯ･鬣ｹ､ﾇ･ｪ｡ｼ･ﾐ｡ｼ･鬣､･ﾉ
    m_LoadProgress = 1.0f;
}

void SceneBase::OnInit() 
{
    // ･ﾇ･ﾕ･ｩ･・ﾈｼﾂﾁ・ ﾇﾉﾀｸ･ｯ･鬣ｹ､ﾇ･ｪ｡ｼ･ﾐ｡ｼ･鬣､･ﾉ
    m_IsInitialized = true;
}

void SceneBase::OnActivate() 
{
    // ･ﾇ･ﾕ･ｩ･・ﾈｼﾂﾁ・ ﾇﾉﾀｸ･ｯ･鬣ｹ､ﾇ･ｪ｡ｼ･ﾐ｡ｼ･鬣､･ﾉ
}

void SceneBase::Update(float deltaTime)
{
    OnUpdate(deltaTime);//ﾇﾉﾀｸ･ｯ･鬣ｹ､ﾎｹｹｿｷ

    // ､ｳ､ﾎ･ｷ｡ｼ･ｬｽ・ｭ､ｹ､・ｪ･ﾖ･ｸ･ｧ･ｯ･ﾈ､ｹｿｷ､ｹ､・
    for (auto it = m_SceneObjects.begin(); it != m_SceneObjects.end(); )
    {
        auto& obj = *it;

        if (!obj->IsPendingDestroy())
        {
            obj->Update(deltaTime);
            ++it;
        }
        else
        {
            // ﾇﾋｴ･ﾕ･鬣ｰ､ｬﾎｩ､ﾃ､ﾆ､､､ｿ､鮨ｪﾎｻｽ靉､ｷ､ﾆｺ・・
            obj->Uninit();
            it = m_SceneObjects.erase(it);
        }
    }
}

void SceneBase::Draw()
{

    OnDraw();//ﾇﾉﾀｸ･ｯ･鬣ｹ､ﾎﾉﾁｲ・

    // ､ｳ､ﾎ･ｷ｡ｼ･ｬｽ・ｭ､ｹ､・D･ｪ･ﾖ･ｸ･ｧ･ｯ･ﾈ､ﾁｲ隍ｹ､・
    for (auto& obj : m_SceneObjects)
    {
        if (obj->Is3D())
        {
            obj->Draw();
        }
    }
}

void SceneBase::DrawUI()
{
    OnDrawUI();

    // ､ｳ､ﾎ･ｷ｡ｼ･ｬｽ・ｭ､ｹ､・D･ｪ･ﾖ･ｸ･ｧ･ｯ･ﾈ(UI)､ﾁｲ隍ｹ､・
    for (auto& obj : m_SceneObjects)
    {
        if (!obj->Is3D())
        {
            obj->Draw();
        }
    }
}

void SceneBase::OnDeactivate() {
    // ･ﾇ･ﾕ･ｩ･・ﾈｼﾂﾁ・ ﾇﾉﾀｸ･ｯ･鬣ｹ､ﾇ･ｪ｡ｼ･ﾐ｡ｼ･鬣､･ﾉ
}

void SceneBase::OnUnload()
{
    ClearSceneObjects();
    m_IsInitialized = false;
}

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

void SceneBase::ClearSceneObjects() 
{
    for (auto& obj : m_SceneObjects) {
        obj->Uninit();
    }
    m_SceneObjects.clear();
}