#pragma once
#include "SceneBase.h"
#include "SceneTransition.h"

// 前方宣言
class IScene;
class SceneTransition;
class SceneLoadingTask;

// シーン識別子�E型定義
using SceneId = std::type_index;

// シーン状慁E
enum class SceneState 
{
    Unloaded,      // 未ローチE
    Loading,       // ロード中
    Ready,         // 準備完亁E
    Active,        // アクチE��チE
    Paused,        // 一時停止
    Transitioning, // 遷移中
    Unloading      // アンロード中
};

// シーンファクトリー
template<typename T>
concept SceneConcept = std::is_base_of_v<IScene, T>;

class SceneFactory {
public:
    template<SceneConcept T>
    static std::unique_ptr<IScene> Create() {
        return std::make_unique<T>();
    }
};

// シーンマネージャー�E�シングルトン�E�E
class SceneManager
{
private:

    // シーン惁E��構造佁E
    struct SceneInfo
    {
        std::unique_ptr<IScene> scene;
        SceneState state = SceneState::Unloaded;
        std::future<void> loadingTask;
        std::chrono::steady_clock::time_point loadStartTime;
        float loadProgress = 0.0f;
    };

    // メンバ変数
    std::unordered_map<SceneId, SceneInfo> m_Scenes;
    std::optional<SceneId> m_CurrentSceneId;
    std::optional<SceneId> m_NextSceneId;
    std::unique_ptr<SceneTransition> m_Transition;
    std::atomic<bool> m_IsTransitioning{ false };

    // シーンファクトリー登録
    std::unordered_map<SceneId, std::function<std::unique_ptr<IScene>()>> m_SceneFactories;

    // ローチE��ング管琁E
    std::queue<std::unique_ptr<SceneLoadingTask>> m_LoadingQueue;
    std::atomic<bool> m_IsLoading{ false };

    // コールバック
    std::function<void(float)> m_OnLoadProgress;
    std::function<void()> m_OnSceneChanged;

    SceneManager() = default;
    ~SceneManager();

public:
    // シングルトンインスタンス取征E
    static SceneManager& GetInstance();

    // コピ�E/ムーブ禁止
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;

    // 初期化�E終亁E
    void Init();
    void Uninit();

    // シーン登録�E�型安�E版！E
    template<SceneConcept T>
    void RegisterScene() 
    {
        SceneId id = std::type_index(typeid(T));
        m_SceneFactories[id] = []() { return SceneFactory::Create<T>(); };
    }

    // シーン操佁E
    template<SceneConcept T>
    void ChangeScene(std::unique_ptr<SceneTransition> transition = nullptr)
    {
        ChangeSceneInternal(std::type_index(typeid(T)), std::move(transition));
    }

    template<SceneConcept T>
    void PreloadScene() 
    {
        PreloadSceneInternal(std::type_index(typeid(T)));
    }

    template<SceneConcept T>
    void UnloadScene() {
        UnloadSceneInternal(std::type_index(typeid(T)));
    }

    // 更新・描画
    void Update(float deltaTime);
    void Draw();
    void DrawUI();  // UI専用描画
    void DrawTransition();

    // 状態取征E
    bool IsTransitioning() const { return m_IsTransitioning; }
    bool IsLoading() const { return m_IsLoading; }
    float GetLoadProgress() const;

    SceneTransition* GetCurrentTransition() const { return m_Transition.get(); }// 現在のシーントランジション取征E

    template<SceneConcept T>
    T* GetScene() {
        SceneId id = std::type_index(typeid(T));
        auto it = m_Scenes.find(id);
        if (it != m_Scenes.end() && it->second.scene) {
            return dynamic_cast<T*>(it->second.scene.get());
        }
        return nullptr;
    }

    // 現在のシーン取征E
    IScene* GetCurrentScene();
    const IScene* GetCurrentScene() const;

    // オブジェクト検索�E�現在のシーンに委譲�E�E
    template<class T>
    std::weak_ptr<T> FindObject() {
        if (auto* scene = dynamic_cast<SceneBase*>(GetCurrentScene())) {
            return scene->FindObject<T>();
        }
        return {};
    }

    template<class T>
    std::vector<std::weak_ptr<T>> FindAllObjects() {
        if (auto* scene = dynamic_cast<SceneBase*>(GetCurrentScene())) {
            return scene->FindAllObjects<T>();
        }
        return {};
    }

    // コールバック設宁E
    void SetLoadProgressCallback(std::function<void(float)> callback) {
        m_OnLoadProgress = callback;
    }

    void SetSceneChangedCallback(std::function<void()> callback) {
        m_OnSceneChanged = callback;
    }

private:
    void ChangeSceneInternal(SceneId sceneId, std::unique_ptr<SceneTransition> transition);
    void PreloadSceneInternal(SceneId sceneId);
    void UnloadSceneInternal(SceneId sceneId);
    void ProcessLoadingQueue();
    void UpdateTransition(float deltaTime);
    void CompleteTransition();
};



// ローチE��ングタスク
class SceneLoadingTask {
public:
    SceneLoadingTask(SceneId id, std::function<void()> loadFunc)
        : m_SceneId(id), m_LoadFunction(loadFunc) {}

    void Execute() { m_LoadFunction(); }
    SceneId GetSceneId() const { return m_SceneId; }

private:
    SceneId m_SceneId;
    std::function<void()> m_LoadFunction;
};

// ローチE��ング画面インターフェース
class ILoadingScreen {
public:
    virtual ~ILoadingScreen() = default;
    virtual void Update(float deltaTime, float progress) = 0;
    virtual void Draw() = 0;
    
};

// チE��ォルトローチE��ング画面
class DefaultLoadingScreen : public ILoadingScreen {
private:
    float m_Progress = 0.0f;
    float m_AnimationTimer = 0.0f;

public:
    void Update(float deltaTime, float progress) override;
    void Draw() override;
};

