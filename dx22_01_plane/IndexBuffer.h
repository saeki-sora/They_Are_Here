#pragma once

#include	<vector>
#include	<wrl/client.h>
#include	"dx11helper.h"
#include	"renderer.h"

using Microsoft::WRL::ComPtr;

//-----------------------------------------------------------------------------
//IndexBufferクラス
//-----------------------------------------------------------------------------
class IndexBuffer {

private:

	ComPtr<ID3D11Buffer> m_IndexBuffer;
	UINT m_IndexCount;  // インデックス数

public:
	void Create(const std::vector<unsigned int>& indices) {

		// デバイス取得
		ID3D11Device* device = nullptr;

		device = Renderer::GetDevice();

		assert(device);

		// インデックスバッファ作成
		bool sts = CreateIndexBuffer(
			device,										// デバイス
			(unsigned int)(indices.size()),				// インデックス数
			(void*)indices.data(),						// インデックスデータ先頭アドレス
			&m_IndexBuffer);							// インデックスバッファ

		assert(sts == true);

		m_IndexCount = static_cast<UINT>(indices.size());
	}

	void SetGPU() {
		// デバイスコンテキスト取得
		ID3D11DeviceContext* devicecontext = nullptr;
		devicecontext = Renderer::GetDeviceContext();

		// インデックスバッファをセット
		devicecontext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	}

	UINT GetIndexCount() const {return m_IndexCount;}

};
