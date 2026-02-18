#pragma once
#include "VisualObject.h"
#include "Texture.h"

class MakeMap;
class Player;

class MiniMap : public VisualObject
{
public:
    MiniMap();
    virtual ~MiniMap() {}
    void Init() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void DrawUI();
    void Uninit() override;

    void SetMap(MakeMap* map);

private:

    // UV調整用の定数バッファ構造体
    struct UVAdjustBuffer
    {
        DirectX::SimpleMath::Vector2 uvOffset;
        DirectX::SimpleMath::Vector2 uvScale;
    };

    void CreateRenderTargets();
    void DrawMapIcons();
    void UpdateFog(const DirectX::SimpleMath::Vector3& playerPos);

    MakeMap* m_MapData = nullptr;
	bool m_IsLargeMap = false;// 大マップ表示フラグ
	bool m_PrevMKey = false;// 前フレームのMキー状態

    // プレイヤーのマップ上UV（0～1）
    float m_PlayerTexX = 0.5f;
    float m_PlayerTexY = 0.5f;

    int m_FogBrushSize = 80;//一度通ったことのある場所はどの程度その周りが見えるようになるか

    // DirectXリソース
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_FogTex, m_MapTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_FogRTV, m_MapRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_FogSRV, m_MapSRV;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_UVAdjustCB;

    // テクスチャ
    Texture m_TexWall;
    Texture m_TexGoal;
    Texture m_TexItem;
    Texture m_TexPlayer;
    Texture m_TexFogBrush;
    Texture m_TexFrame;

    // シェーダー
    std::unique_ptr<Shader> m_MiniMapShader;

	// 事前描画用リソース
    Microsoft::WRL::ComPtr<ID3D11Texture2D>			m_StaticMapTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_StaticMapRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_StaticMapSRV;

    // 事前描画用関数
    void CreateStaticMapResources(); // リソース作成
    void BakeStaticMap();            // 壁を書き込む関数
};