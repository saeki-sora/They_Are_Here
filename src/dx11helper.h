#pragma once


HRESULT CompileShader(const char* szFileName, 
	LPCSTR szEntryPoint, 
	LPCSTR szShaderModel, 
	void** ShaderObject, 
	size_t& ShaderObjectSize, 
	ID3DBlob** ppBlobOut);

HRESULT CompileShaderFromFile(
	const char* szFileName, 
	LPCSTR szEntryPoint, 
	LPCSTR szShaderModel, 
	ID3DBlob** ppBlobOut);

bool CreateConstantBuffer(
	ID3D11Device* device,
	unsigned int bytesize,
	ID3D11Buffer** pConstantBuffer			// コンスタントバッファ
	);

bool CreateConstantBufferWrite(
	ID3D11Device* device,					// デバイスオブジェクト
	unsigned int bytesize,					// コンスタントバッファサイズ
	ID3D11Buffer** pConstantBuffer			// コンスタントバッファ
);

bool CreateIndexBuffer(
	ID3D11Device* device,
	unsigned int indexnum,					// 頂点数
	void* indexdata,						// インデックスデータ格納メモリ先頭アドレス
	ID3D11Buffer** pIndexBuffer);

bool CreateVertexBuffer(
	ID3D11Device* device,
	unsigned int stride,				// １頂点当たりバイト数
	unsigned int vertexnum,				// 頂点数
	void* vertexdata,					// 頂点データ格納メモリ先頭アドレス
	ID3D11Buffer** pVertexBuffer		// 頂点バッファ
	);

bool CreateVertexBufferWrite(
	ID3D11Device* device,
	unsigned int stride,				// １頂点当たりバイト数
	unsigned int vertexnum,				// 頂点数
	void* vertexdata,					// 頂点データ格納メモリ先頭アドレス
	ID3D11Buffer** pVertexBuffer		// 頂点バッファ
);

bool CreateVertexBufferUAV(
	ID3D11Device* device,
	unsigned int stride,				// １頂点当たりバイト数
	unsigned int vertexnum,				// 頂点数
	void* vertexdata,					// 頂点データ格納メモリ先頭アドレス
	ID3D11Buffer** pVertexBuffer		// 頂点バッファ
	);

bool CreateStructuredBuffer(
	ID3D11Device* device,
	unsigned int stride,				// ストライドバイト数
	unsigned int num,					// 個数
	void* data,							// データ格納メモリ先頭アドレス
	ID3D11Buffer** pStructuredBuffer	// StructuredBuffer
	);

ID3D11Buffer* CreateAndCopyToBuffer(
	ID3D11Device* device,
	ID3D11DeviceContext* devicecontext,
	ID3D11Buffer* pBuffer	// RWStructuredBuffer
	);

bool CreateShaderResourceView(			// バッファからシェーダーリソースビューを作成する	
	ID3D11Device* device,
	ID3D11Buffer* pBuffer,				// Buffer
	ID3D11ShaderResourceView** ppSRV);

bool CreateUnOrderAccessView(
	ID3D11Device* device,
	ID3D11Buffer* pBuffer,				// Buffer
	ID3D11UnorderedAccessView** ppUAV);

bool CreateVertexShader(ID3D11Device* device,		// 頂点シェーダーオブジェクトを生成、同時に頂点レイアウトも生成
	const char* szFileName,
	LPCSTR szEntryPoint,
	LPCSTR szShaderModel,
	D3D11_INPUT_ELEMENT_DESC* layout,
	unsigned int numElements,
	ID3D11VertexShader** ppVertexShader,
	ID3D11InputLayout**  ppVertexLayout);


bool CreatePixelShader(ID3D11Device* device,		// ピクセルシェーダーオブジェクトを生成
	const char* szFileName,
	LPCSTR szEntryPoint,
	LPCSTR szShaderModel,
	ID3D11PixelShader** ppPixelShader);

std::string ExtractFileName(std::string fullpath, char split);

// デバッグ描画時の D3D11 レンダーステート一時保存用
// SimpleBoxCollider と Enemy の DrawDebug 系関数で共有する
struct D3D11DebugState
{
	ID3D11RasterizerState*    RasterState                                       = nullptr;
	ID3D11DepthStencilState*  DepthState                                        = nullptr;
	UINT                      StencilRef                                         = 0;
	ID3D11BlendState*         BlendState                                        = nullptr;
	float                     BlendFactor[4]                                    = {};
	UINT                      SampleMask                                        = 0;
	ID3D11InputLayout*        InputLayout                                       = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY  Topology                                          = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	ID3D11VertexShader*       VS                                                = nullptr;
	ID3D11ClassInstance*      VSInstances[256]                                  = {};
	UINT                      NumVSInst                                         = 256;
	ID3D11PixelShader*        PS                                                = nullptr;
	ID3D11ClassInstance*      PSInstances[256]                                  = {};
	UINT                      NumPSInst                                         = 256;
	ID3D11Buffer*             VB[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]     = {};
	UINT                      VBStride[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	UINT                      VBOffset[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	ID3D11Buffer*             IB                                                = nullptr;
	DXGI_FORMAT               IBFmt                                             = DXGI_FORMAT_UNKNOWN;
	UINT                      IBOfs                                             = 0;
	ID3D11Buffer*             VSCB[4]                                           = {};
	ID3D11Buffer*             PSCB[4]                                           = {};
	ID3D11ShaderResourceView* PSSRV[4]                                          = {};
	ID3D11SamplerState*       PSSampler[4]                                      = {};
};

// デバッグ描画前後に呼ぶ。Save → 描画 → Restore の組み合わせで使う
void SaveD3D11DebugState(ID3D11DeviceContext* ctx, D3D11DebugState& state);
void RestoreD3D11DebugState(ID3D11DeviceContext* ctx, D3D11DebugState& state);