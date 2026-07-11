#include "pch.h"
#include "ImGUI_Manager.h"
#include "ConfigManager.h"
#include "SceneManager.h"
#include "Enemy.h"
#include "EnemyDebugData.h"
#include "PostProcessManager.h"
#include "Game.h"
#include "Application.h"
#include "DebugManager.h"

// 日本語表示用フォント
// 実行環境のWindowsに依存しないよう、再配布可能なフォントをプロジェクトに同梱している（OFLライセンス）
static constexpr const char* JP_FONT_PATH = "assets/font/ZenMaruGothic-Regular.ttf";

bool ImGUI_Manager::s_Initialized     = false;
bool ImGUI_Manager::s_Visible         = false;
bool ImGUI_Manager::s_CursorVisible   = true;
int  ImGUI_Manager::s_SelectedEnemyUID = -1;

// ============================================================
// 初期化
// ============================================================
void ImGUI_Manager::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 実解像度は2D UIの仮想解像度より大きくなり得るため、フォントとスタイルサイズを実解像度に合わせて拡大する
    // （そのままだとフルスクリーンの実解像度上で文字やスライダーが小さく表示されてしまう）
    const float uiScale = static_cast<float>(Application::GetWidth()) / static_cast<float>(Application::VIRTUAL_WIDTH);
    const float fontSize = 16.0f * uiScale; // 日本語は既定の13pxだと潰れやすいため少し大きめを基準にする

    // 日本語フォントを読み込む（文字化け防止。既定フォントは半角英数字にしか対応していないため）
    ImFont* jpFont = nullptr;
    if (std::filesystem::exists(JP_FONT_PATH))
    {
        jpFont = io.Fonts->AddFontFromFileTTF(JP_FONT_PATH, fontSize, nullptr, io.Fonts->GetGlyphRangesJapanese());
    }
    if (!jpFont)
    {
        // フォントファイルが同梱されていない実行環境向けのフォールバック（この場合、日本語表示は文字化けする）
        ImFontConfig fallbackConfig;
        fallbackConfig.SizePixels = fontSize;
        io.Fonts->AddFontDefault(&fallbackConfig);
    }

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(uiScale); // パディング・スライダー等のウィジェットサイズもスケーリング

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

	s_Initialized = true;// 初期化完了後はUIを非表示にしておく
}

// ============================================================
// フレーム開始
// ============================================================
void ImGUI_Manager::BeginFrame()
{
    if (!s_Initialized) return;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

// ============================================================
// 全パネルをまとめて描画する
// ============================================================
void ImGUI_Manager::DrawPanels()
{
    if (!s_Initialized) return;

	//PキーでデバッグUIの表示/非表示を切り替える
    static bool s_PPrev = false;
    bool pNow = (GetAsyncKeyState('P') & 0x8000) != 0;

    if (pNow && !s_PPrev)
    {
        s_Visible = !s_Visible;

        // パネル表示中はマウスをImGui操作に使うため、FPSカメラのキャプチャを解放する
        Camera& cam = Game::GetInstance().GetMainCamera();
        if (s_Visible)
        {
            cam.ReleaseMouseImmediate();
            cam.SetClickToRecapture(false); // パネル操作中はクリックで誤キャプチャされないよう無効化
            s_CursorVisible = true; // パネルを開いた直後はカーソルを表示した状態にする
        }
        else
        {
            cam.SetClickToRecapture(true);
            cam.RecaptureMouseImmediate();
        }
    }
    s_PPrev = pNow;

    if (!s_Visible) return;

    // Iキーでマウスカーソルの表示/非表示を切り替える（デバッグUI表示中のみ）
    // カーソル表示中は視点操作を無効化し、非表示中は視点操作を有効化する
    static bool s_IPrev = false;
    bool iNow = (GetAsyncKeyState('I') & 0x8000) != 0;

    if (iNow && !s_IPrev)
    {
        s_CursorVisible = !s_CursorVisible;

        Camera& cam = Game::GetInstance().GetMainCamera();
        if (s_CursorVisible) cam.ReleaseMouseImmediate();   // カーソル表示 → 視点操作OFF
        else                 cam.RecaptureMouseImmediate(); // カーソル非表示 → 視点操作ON
    }
    s_IPrev = iNow;

    DrawEnemyParamPanel();
    DrawFSMPanel();
    DrawGraphicsPanel();
    DrawDebugHelpPanel();
}

// ============================================================
// グラフィック設定パネルを描画
// ============================================================
void ImGUI_Manager::DrawGraphicsPanel()
{
    ImGui::Begin("グラフィック設定");
    PostProcessManager::GetInstance().DrawImGui();
    ImGui::End();
}

// ============================================================
// デバッグ操作方法パネルを描画（初めてデバッグ機能を使う人向けの早見表）
// ============================================================
void ImGUI_Manager::DrawDebugHelpPanel()
{
    ImGui::SetNextWindowPos(ImVec2(10, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("デバッグ操作説明"))
    {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("デバッグUI表示中に使えるキー操作の一覧です。");
    ImGui::Separator();

    DebugManager& dbg = DebugManager::GetInstance();

    // 1行分の「キー / 説明 / 状態(ON/OFF)」を描画するヘルパー
    auto row = [](const char* key, const char* action, const bool* state)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(key);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextWrapped("%s", action);
        ImGui::TableSetColumnIndex(2);
        if (state)
        {
            if (*state) ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "ON");
            else        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "OFF");
        }
    };

    if (ImGui::BeginTable("debug_help_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("キー",   ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("操作内容", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("状態",   ImGuiTableColumnFlags_WidthFixed, 45.0f);
        ImGui::TableHeadersRow();

        bool debugOn = dbg.IsDebugModeEnabled();
        row("P", "デバッグUIの表示切り替え（マウス操作も可能になる）", &debugOn);
        row("I", "マウスカーソルの表示/非表示切り替え（UI表示中のみ有効）", &s_CursorVisible);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("-- 以下はデバッグUI（Pキー）がONの間だけ有効 --");

        bool colliders  = dbg.ShouldShowColliders();
        bool noClip     = dbg.IsNoClipMode();
        bool path       = dbg.ShouldShowEnemyPath();
        bool vision     = dbg.ShouldShowEnemyVision();
        bool invincible = dbg.IsInvincibleMode();
        bool whisker    = dbg.ShouldShowEnemyWhisker();
        bool lamps      = dbg.ShouldShowLampLights();

        row("F1", "当たり判定（コライダー）のワイヤーフレーム表示切り替え", &colliders);
        row("F2", "ノークリップモード切り替え（壁をすり抜けて自由に移動）", &noClip);
        row("F3", "敵の経路探索ルート表示切り替え",                       &path);
        row("F4", "敵の視界（視野範囲）表示切り替え",                     &vision);
        row("F5", "無敵モード切り替え（敵に捕まらなくなる）",             &invincible);
        row("F6", "敵のウィスカー（曲がり角回避センサー）表示切り替え",   &whisker);
        row("F7", "ランプの点光源位置表示切り替え",                       &lamps);

        ImGui::EndTable();
    }

    ImGui::End();
}

// ============================================================
// 描画
// ============================================================
void ImGUI_Manager::Render()
{
    if (!s_Initialized) return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGUI_Manager::Uninit()
{
    if (!s_Initialized) return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    s_Initialized = false;
}

// ============================================================
// 敵パラメータパネルを描画
// ============================================================
void ImGUI_Manager::DrawEnemyParamPanel()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 480), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("敵パラメータ"))
    {
        ImGui::End();
        return;
    }

	json& data = ConfigManager::GetInstance().GetDataMutable();
    if (!data.is_object())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "enemy_param.json が読み込まれていません。");
        ImGui::End();
        return;
    }

    // --- Common ---
    if (ImGui::CollapsingHeader("共通", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& common = data["Common"];

        // JSONのdouble型をfloat型に変換して格納する
        float searchSpeed = common.value("SearchSpeed", 45.0f);
        if (ImGui::SliderFloat("索敵速度", &searchSpeed, 5.0f, 200.0f, "%.1f"))
            common["SearchSpeed"] = searchSpeed;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("巡回中の移動速度");

        float chaseSpeed = common.value("ChaseSpeed", 50.0f);
        if (ImGui::SliderFloat("追跡速度", &chaseSpeed, 5.0f, 200.0f, "%.1f"))
            common["ChaseSpeed"] = chaseSpeed;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("プレイヤーを追跡している間の速度");

        float maxAccel = common.value("MaxAccel", 60.0f);
        if (ImGui::SliderFloat("最大加速度", &maxAccel, 5.0f, 300.0f, "%.1f"))
            common["MaxAccel"] = maxAccel;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("加速の強さ（大きいほど最高速度に早く到達する）");

        float maxAngVelDeg = common.value("MaxAngVelDeg", 320.0f);
        if (ImGui::SliderFloat("最大旋回速度 (度/秒)", &maxAngVelDeg, 10.0f, 720.0f, "%.0f"))
            common["MaxAngVelDeg"] = maxAngVelDeg;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("1秒あたりの最大回転角度");
    }

    // --- Detection ---
    if (ImGui::CollapsingHeader("索敵", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& detect = data["Detection"];

        float radius = detect.value("Radius", 800.0f);
        if (ImGui::SliderFloat("視界半径", &radius, 50.0f, 2000.0f, "%.0f"))
            detect["Radius"] = radius;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("敵が見える距離（ワールド単位）");

        float fov = detect.value("FOV", 90.0f);
        if (ImGui::SliderFloat("視野角 (度)", &fov, 10.0f, 180.0f, "%.0f"))
            detect["FOV"] = fov;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("視野の角度（度数法）");

        float catchRange = detect.value("CatchRange", 30.0f);
        if (ImGui::SliderFloat("捕捉距離", &catchRange, 5.0f, 150.0f, "%.0f"))
            detect["CatchRange"] = catchRange;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("プレイヤーが捕まる距離");

        float lostDuration = detect.value("LostDuration", 5.0f);
        if (ImGui::SliderFloat("見失い継続時間 (秒)", &lostDuration, 0.5f, 30.0f, "%.1f"))
            detect["LostDuration"] = lostDuration;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("見失ってから諦めるまで捜索を続ける時間");
    }

    // --- AI ---
    if (ImGui::CollapsingHeader("AI", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& ai = data["AI"];

        float chaseInterval = ai.value("ChasePathUpdateInterval", 0.25f);
        if (ImGui::SliderFloat("経路再計算間隔 (秒)", &chaseInterval, 0.05f, 2.0f, "%.2f"))
            ai["ChasePathUpdateInterval"] = chaseInterval;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("追跡中に経路を再計算する頻度");

        float cornerCut = ai.value("CornerCutDist", 15.0f);
        if (ImGui::SliderFloat("コーナーカット距離", &cornerCut, 0.0f, 60.0f, "%.1f"))
            ai["CornerCutDist"] = cornerCut;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("曲がり角の手前どれくらいから曲がり始めるか");
    }

    ImGui::Separator();

    // 敵パラメータを全ての敵に適用するボタン
    if (ImGui::Button("全ての敵に適用", ImVec2(-1, 0)))
    {
        auto enemies = SceneManager::GetInstance().FindAllObjects<Enemy>();
        for (auto& weak : enemies)
        {
            if (auto e = weak.lock())
                e->ReloadParams();
        }
        ImGui::OpenPopup("applied_popup");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("現在の値を全ての出現中の敵に即座に反映する");

	// ポップアップで適用完了のメッセージを表示
    if (ImGui::BeginPopup("applied_popup"))
    {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d体の敵に適用しました！",
            (int)SceneManager::GetInstance().FindAllObjects<Enemy>().size());
        ImGui::EndPopup();
    }

    ImGui::Spacing();

    // Saveボタンを押すと現在の設定をJSONファイルに保存
    if (ImGui::Button("JSONに保存", ImVec2(-1, 0)))
    {
        ConfigManager::GetInstance().Save("json/enemy_param.json");
        ImGui::OpenPopup("saved_popup");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("現在の値を enemy_param.json に書き込む");

    if (ImGui::BeginPopup("saved_popup"))
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "enemy_param.json に保存しました");
        ImGui::EndPopup();
    }

    ImGui::End();
}

// ============================================================
// 敵パラメータパネルを描画
// ============================================================
static void StateStyle(const std::string& name, const char*& label, ImU32& color)
{
    if      (name == "EnemySearchState")         { label = "Search";       color = IM_COL32(80,  160, 80,  255); }
    else if (name == "EnemyChaseState")          { label = "Chase";        color = IM_COL32(200, 80,  80,  255); }
    else if (name == "EnemyLostState")           { label = "Lost";         color = IM_COL32(80,  80,  200, 255); }
    else if (name == "EnemyState_StalkerChase")  { label = "StalkerChase"; color = IM_COL32(160, 40,  200, 255); }
    else if (name == "EnemyState_loversChase")   { label = "LoversChase";  color = IM_COL32(200, 160, 40,  255); }
    else                                         { label = "Unknown";      color = IM_COL32(120, 120, 120, 255); }
}

// ============================================================
// FSMのノードを描画
// ============================================================
static void DrawNode(ImDrawList* dl, ImVec2 center, float r,
                     const char* label, ImU32 color, bool active)
{
    ImU32 fill   = active ? color : IM_COL32(50, 50, 50, 255);
    ImU32 border = active ? IM_COL32(255, 230, 50, 255) : IM_COL32(120, 120, 120, 255);
    float thick  = active ? 3.0f : 1.5f;

    dl->AddCircleFilled(center, r, fill, 32);
    dl->AddCircle(center, r, border, 32, thick);

    // テキストを中央に配置
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
                IM_COL32(220, 220, 220, 255), label);
}

// ============================================================
// 矢印を描画
// ============================================================
static void DrawArrow(ImDrawList* dl, ImVec2 a, ImVec2 b, float nodeR, ImU32 color)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;
    float nx = dx / len, ny = dy / len;

    // ノードの半径を考慮して始点と終点を調整
    ImVec2 from = ImVec2(a.x + nx * nodeR, a.y + ny * nodeR);
    ImVec2 to   = ImVec2(b.x - nx * nodeR, b.y - ny * nodeR);

    dl->AddLine(from, to, color, 1.5f);

    // 矢印の先端を描画
    float headLen = 8.0f;
    float ax =  nx * headLen * 0.866f + ny * headLen * 0.5f;
    float ay = -nx * headLen * 0.5f   + ny * headLen * 0.866f;
    dl->AddLine(to, ImVec2(to.x - ax, to.y - ay), color, 1.5f);
    dl->AddLine(to, ImVec2(to.x - ay, to.y + ax), color, 1.5f);
}

// ============================================================
// 敵の状態遷移グラフを描画
// ============================================================
void ImGUI_Manager::DrawFSMGraph(const std::string& currentState)
{
    const float NODE_R    = 24.0f;
    const float GRAPH_W   = 360.0f;
    const float GRAPH_H   = 90.0f;
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // ImDrawListを使ってImGuiの描画コマンドを発行するための準備
    ImGui::Dummy(ImVec2(GRAPH_W, GRAPH_H));

    // 各状態のノード位置
    ImVec2 searchPos = ImVec2(origin.x + 70,  origin.y + 60);
    ImVec2 chasePos  = ImVec2(origin.x + 185, origin.y + 60);
    ImVec2 lostPos   = ImVec2(origin.x + 300, origin.y + 60);

    // --- Search ---
    ImU32 edgeCol = IM_COL32(140, 140, 140, 180);
    DrawArrow(dl, searchPos, chasePos, NODE_R, edgeCol); // Search -> Chase
    DrawArrow(dl, chasePos, lostPos,  NODE_R, edgeCol); // Chase  -> Lost
    DrawArrow(dl, lostPos,  searchPos, NODE_R, edgeCol); // Lost   -> Search
    DrawArrow(dl, chasePos, searchPos, NODE_R, edgeCol); // Chase  -> Search

    // --- Detection ---
    const char* label; ImU32 col;

    StateStyle("EnemySearchState", label, col);
    DrawNode(dl, searchPos, NODE_R, label, col, currentState == "EnemySearchState");

    StateStyle("EnemyChaseState", label, col);
    DrawNode(dl, chasePos, NODE_R, label, col,
             currentState == "EnemyChaseState" ||
             currentState == "EnemyState_StalkerChase" ||
             currentState == "EnemyState_loversChase");

    StateStyle("EnemyLostState", label, col);
    DrawNode(dl, lostPos, NODE_R, label, col, currentState == "EnemyLostState");

    // ステータスがChaseの時はその状態を強調表示
    if (currentState == "EnemyState_StalkerChase" || currentState == "EnemyState_loversChase")
    {
        ImGui::TextDisabled("(現在の状態: %s)", currentState.c_str());
    }
}

// ============================================================
// 状態遷移タイムラインを描画
// 1フレームに1つの状態遷移を記録
// ============================================================
void ImGUI_Manager::DrawTransitionTimeline(int uid)
{
	// UIDに紐づく状態遷移履歴を取得
    auto& reg = EnemyDebugData::Registry();
    auto it = reg.find(uid);
    if (it == reg.end() || it->second.history.empty())
    {
        ImGui::TextDisabled("まだ状態遷移が記録されていません。");
        return;
    }

	// 状態遷移履歴を取得
	const auto& history = it->second.history;// 取得した履歴
    ImDrawList* dl      = ImGui::GetWindowDrawList();
    ImVec2 origin       = ImGui::GetCursorScreenPos();

    const float BAR_H   = 22.0f;
    const float TOTAL_W = 360.0f;
    const float BAR_W   = TOTAL_W / EnemyDebugData::HISTORY_SIZE;

	ImGui::Dummy(ImVec2(TOTAL_W, BAR_H + 16.0f));// スペースを確保

	int startSlot = EnemyDebugData::HISTORY_SIZE - (int)history.size();// 描画する開始スロット

    for (int i = 0; i < (int)history.size(); ++i)
    {
		// イベント情報を取得
        const auto& ev = history[i];
        const char* label; ImU32 col;
        StateStyle(ev.stateName, label, col);

		// テキストを中央に配置
        float x0 = origin.x + (startSlot + i) * BAR_W;
        float y0 = origin.y;
		float x1 = x0 + BAR_W - 1.0f; // 右端を1pxずらす(重なりを防止)
        float y1 = y0 + BAR_H;

        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), col, 3.0f);

		// テキストを中央に配置
        if (BAR_W > 18.0f)
        {
            ImVec2 ts = ImGui::CalcTextSize(label);
            if (ts.x < BAR_W - 2.0f)
            {
                dl->AddText(
                    ImVec2(x0 + (BAR_W - ts.x) * 0.5f, y0 + (BAR_H - ts.y) * 0.5f),
                    IM_COL32(255, 255, 255, 220), label);
            }
        }
    }

    // 最新の状態遷移を表示
    const auto& last = history.back();
    ImGui::TextDisabled("最新: %s  (経過時間 %.2f秒)", last.stateName.c_str(), last.gameTime);
}

// ============================================================
// FSM状態遷移グラフを描画
// 敵の状態遷移を可視化するためのUI
// ============================================================
void ImGUI_Manager::DrawFSMPanel()
{
    ImGui::SetNextWindowPos(ImVec2(400, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("状態遷移ビジュアライザ"))
    {
        ImGui::End();
        return;
    }

    auto& reg = EnemyDebugData::Registry();
    if (reg.empty())
    {
        ImGui::TextDisabled("敵がまだ出現していません。");
        ImGui::End();
        return;
    }

    // collect and sort UIDs
    std::vector<int> uids;
    uids.reserve(reg.size());
    for (auto& pair : reg)
        uids.push_back(pair.first);
    std::sort(uids.begin(), uids.end());

    // reset selection if stale
    if (s_SelectedEnemyUID == -1 || reg.find(s_SelectedEnemyUID) == reg.end())
        s_SelectedEnemyUID = uids.front();

    // list layout constants
    const float ROW_H     = 22.0f;
    const float BADGE_SZ  = 14.0f;
    const float DOT_R     = 4.0f;
    const int   MAX_ROWS  = 7;
    const int   DOT_COUNT = 8;

    int visibleRows = (int)uids.size();
    if (visibleRows > MAX_ROWS) visibleRows = MAX_ROWS;
    float listH = visibleRows * ROW_H;

    ImGui::BeginChild("enemy_list", ImVec2(0, listH), false, 0);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int i = 0; i < (int)uids.size(); ++i)
    {
        int uid = uids[i];
        auto& entry = reg[uid];
        const char* stateLabel; ImU32 stateCol;
        StateStyle(entry.currentStateName, stateLabel, stateCol);

        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        float rowW    = ImGui::GetContentRegionAvail().x;

        // highlight selected row
        if (uid == s_SelectedEnemyUID)
            dl->AddRectFilled(rowMin, ImVec2(rowMin.x + rowW, rowMin.y + ROW_H),
                              IM_COL32(60, 80, 120, 180));

        // state color badge
        float badgeX = rowMin.x + 4.0f;
        float badgeY = rowMin.y + (ROW_H - BADGE_SZ) * 0.5f;
        dl->AddRectFilled(ImVec2(badgeX, badgeY),
                          ImVec2(badgeX + BADGE_SZ, badgeY + BADGE_SZ),
                          stateCol, 2.0f);

        // "Enemy #N  [State]" text
        char rowText[64];
        snprintf(rowText, sizeof(rowText), "敵 #%d  [%s]", uid, stateLabel);
        dl->AddText(ImVec2(rowMin.x + BADGE_SZ + 10.0f,
                           rowMin.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f),
                    IM_COL32(220, 220, 220, 255), rowText);

        // history dots right-aligned (oldest left, newest right)
        {
            const auto& hist = entry.history;
            int dotStart = (int)hist.size() - DOT_COUNT;
            if (dotStart < 0) dotStart = 0;
            int dotCount = (int)hist.size() - dotStart;
            float dotsAreaW = DOT_COUNT * (DOT_R * 2.0f + 2.0f);
            float dotX = rowMin.x + rowW - dotsAreaW - 4.0f;
            float dotY = rowMin.y + ROW_H * 0.5f;
            for (int d = 0; d < dotCount; ++d)
            {
                const char* dotLabel; ImU32 dc;
                StateStyle(hist[dotStart + d].stateName, dotLabel, dc);
                float cx = dotX + d * (DOT_R * 2.0f + 2.0f) + DOT_R;
                dl->AddCircleFilled(ImVec2(cx, dotY), DOT_R, dc, 8);
            }
        }

        // click to select
        ImGui::InvisibleButton(("##row" + std::to_string(uid)).c_str(), ImVec2(rowW, ROW_H));
        if (ImGui::IsItemClicked())
            s_SelectedEnemyUID = uid;
    }

    ImGui::EndChild();

    ImGui::Separator();

    // node graph for selected enemy
    const std::string& cur = reg[s_SelectedEnemyUID].currentStateName;
    DrawFSMGraph(cur);

    ImGui::Separator();

    // transition timeline for selected enemy
    ImGui::Text("状態遷移履歴（古い → 新しい）");
    DrawTransitionTimeline(s_SelectedEnemyUID);

    ImGui::End();
}
