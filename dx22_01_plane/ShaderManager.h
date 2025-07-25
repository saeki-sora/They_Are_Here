#pragma once
#include <memory>
#include <unordered_map>
#include "Shader.h"
class ShaderManager
{
public:
	static ShaderManager& GetInstance()
	{
		static ShaderManager instance;
		return instance;
	}

	std::shared_ptr<Shader> GetShader(std::string vs, std::string ps);
private:
	std::unordered_map<std::string, std::shared_ptr<Shader>> m_ShaderCache;
	ShaderManager();
	~ShaderManager();
};

