//#include "Skybox.h"
//#include "Renderer.h"
//#include "DDSTextureLoader.h"
//#include "Game.h"
//
//
//Skybox::Skybox()
//	: VisualObject(DirectX::SimpleMath::Vector3(0, 0, 0), DirectX::SimpleMath::Vector3(500, 500, 500)) // サイズは適宜調整
//{}
//
//Skybox::~Skybox() {}
//
//void Skybox::Init()
//{
//	// キューブマップ用シェーダをセットアップ
//	m_Shader.Create("shader/skyboxVS.hlsl", "shader/skyboxPS.hlsl");
//
//	// サンプラー設定
//	D3D11_SAMPLER_DESC samplerDesc{};
//	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
//	samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
//	Renderer::GetDevice()->CreateSamplerState(&samplerDesc, &m_SamplerState);
//
//	// 深度テスト無効化設定
//	D3D11_DEPTH_STENCIL_DESC dsDesc{};
//	dsDesc.DepthEnable = FALSE;
//	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
//	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
//	Renderer::GetDevice()->CreateDepthStencilState(&dsDesc, &m_DepthStencilState);
//
//}
//
//void Skybox::Update()
//{
//	// カメラ位置を取得してスカイボックスの中心位置にする
//	Camera& camera = Game::GetInstance().GetMainCamera();
//	SetPosition(camera.GetPosition());
//}
//
//
//void Skybox::Draw()
//{
//	ID3D11DeviceContext* context = Renderer::GetDeviceContext();
//
//	// 深度テスト無効化
//	context->OMSetDepthStencilState(m_DepthStencilState, 0);
//
//	// ビュー行列から平行移動成分を除去
//	Matrix view = Renderer::GetViewMatrix();
//	view._41 = view._42 = view._43 = 0.0f;
//
//	Matrix projection = Renderer::GetProjectionMatrix();
//	Matrix world = Matrix::CreateScale(m_Scale) * Matrix::CreateFromYawPitchRoll(m_Rotation.x, m_Rotation.y, m_Rotation.z) * Matrix::CreateTranslation(m_Position);
//
//	Renderer::SetWorldMatrix(&world);
//	Renderer::SetViewMatrix(&view);
//	Renderer::SetProjectionMatrix(&projection);
//
//	// シェーダ、バッファ、キューブマップ、サンプラーをセット
//	m_Shader.SetGPU();
//	m_VertexBuffer.SetGPU();
//	m_IndexBuffer.SetGPU();
//
//	context->PSSetShaderResources(0, 1, &m_CubeMapSRV);
//	context->PSSetSamplers(0, 1, &m_SamplerState);
//
//	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//	context->DrawIndexed(m_IndexBuffer.GetIndexCount(), 0, 0);
//
//
//	//ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
//	//context->PSSetShaderResources(0, 1, nullSRV);
//
//	//ID3D11SamplerState* nullSampler[1] = { nullptr };
//	//context->PSSetSamplers(0, 1, nullSampler);
//
//	// 深度テスト状態を戻す
//	context->OMSetDepthStencilState(nullptr, 0);
//
//}
//
//
//void Skybox::LoadCubeMap(const wchar_t* filename)
//{
//	HRESULT hr = DirectX::CreateDDSTextureFromFile(
//		Renderer::GetDevice(),
//		filename,
//		nullptr,
//		&m_CubeMapSRV);
//	assert(SUCCEEDED(hr));
//}
//
//void Skybox::Uninit()
//{
//	if (m_CubeMapSRV) m_CubeMapSRV->Release();
//	if (m_SamplerState) m_SamplerState->Release();
//	if (m_DepthStencilState) m_DepthStencilState->Release();
//}
