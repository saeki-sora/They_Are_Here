#pragma once
#include	"ShaderManager.h"

using Microsoft::WRL::ComPtr;
struct ShaderData;// 前方宣言

//-----------------------------------------------------------------------------
//Shaderクラス
//-----------------------------------------------------------------------------
class Shader{

private:

	std::shared_ptr<ShaderData> m_shaderData; // シェーダーデータへの参照

public:

	void Create(std::string vs, std::string ps);
	void SetGPU();

	bool IsValid() const { return m_shaderData != nullptr; }

};

